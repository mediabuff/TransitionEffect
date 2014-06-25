#include "pch.h"
#include "CSource.h"
#include "helpers.h"

using namespace AdvancedMediaSource;

CSource::CSource(IMFDXGIDeviceManager* pDXManager, HANDLE deviceHandle, Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect)
	: m_Initialized(false)
{
	m_Intro.Type = intro;
	m_Intro.Duration = introDuration * 10000;
	m_Outro.Type = outro;
	m_Outro.Duration = outroDuration * 10000;

	HRESULT hr;
	ComPtr<IMFAttributes> m_pAttributes;

	m_vd.HasAudio = false;

	if(MFCreateAttributes(&m_pAttributes, 10) != S_OK)
	{
		return;
	}

	//m_pAttributes->SetUINT32(MF_LOW_LATENCY, 1);
	m_pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
	m_pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
	//m_pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_TRANSCODE_ONLY_TRANSFORMS, TRUE);
	m_pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, pDXManager);
	
	if(MFCreateSourceReaderFromURL(url->Data(), m_pAttributes.Get(), &m_pSourceReader) != S_OK)
	{
		return;
	}

	ComPtr<IMFMediaSource> pMediaSource;
	ComPtr<IMFPresentationDescriptor> pPresentationDesc;	

	if(m_pSourceReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, GUID_NULL, IID_PPV_ARGS(&pMediaSource)) != S_OK)
	{
		return;
	}

	if(pMediaSource->CreatePresentationDescriptor(&pPresentationDesc) != S_OK)
	{
		return;
	}

	if(pPresentationDesc->GetUINT64(MF_PD_DURATION, (UINT64*) &m_vd.Duration) != S_OK)
	{
		return;
	}

	for(DWORD streamId = 0; true; streamId++)
	{
		ComPtr<IMFMediaType> pNativeType;
		ComPtr<IMFMediaType> pType;
		ComPtr<IMFMediaType> pFinalType;
		GUID majorType, subtype;

		hr = m_pSourceReader->GetNativeMediaType(streamId, 0, &pNativeType);
		if(hr == MF_E_NO_MORE_TYPES)
		{
			continue;
		}
		else if(hr == MF_E_INVALIDSTREAMNUMBER)
		{
			break;
		}
		else if(FAILED(hr))
		{
			return;
		}

		// Find the major type.
		hr = pNativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
		if(FAILED(hr))
		{
			return;
		}

		if(pNativeType->GetGUID(MF_MT_SUBTYPE, &subtype) != S_OK)
		{
			return;
		}

		if(MFCreateMediaType(&pType) != S_OK)
		{
			return;
		}

		if(pType->SetGUID(MF_MT_MAJOR_TYPE, majorType) != S_OK)
		{
			return;
		}

		// Select a subtype.
		if(majorType == MFMediaType_Video)
		{
			subtype = MFVideoFormat_ARGB32;
				 //MFVideoFormat_NV12;
		}
		else if(majorType == MFMediaType_Audio)
		{
			subtype = MFAudioFormat_PCM;
		}
		else
		{			
			continue; // Unrecognized type. Skip.
		}

		if(pType->SetGUID(MF_MT_SUBTYPE, subtype) != S_OK)
		{
			return;
		}

		//pType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);

		hr = m_pSourceReader->SetCurrentMediaType(streamId, NULL, pType.Get());
		if(FAILED(hr))
		{
			return;
		}

		// test final type
		hr = m_pSourceReader->GetCurrentMediaType(streamId, &pFinalType);
		if(FAILED(hr))
		{
			return;
		}

		DWORD eFlags = 0;

		if(pType->IsEqual(pFinalType.Get(), &eFlags) == E_INVALIDARG)
		{
			return;
		}
		else
		{
			if(!(eFlags & (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES)))
			{
				return;
			}
		}

		if(majorType == MFMediaType_Video)
		{
			if(MFGetAttributeSize(pFinalType.Get(), MF_MT_FRAME_SIZE, &m_vd.Width, &m_vd.Height) != S_OK)
			{
				return;
			}

			if(MFGetAttributeRatio(pFinalType.Get(), MF_MT_FRAME_RATE, &m_vd.Numerator, &m_vd.Denominator) != S_OK)
			{
				return ;
			}

			m_vd.Stride = m_vd.Width * 4;
			m_vd.SampleSize = m_vd.Stride * m_vd.Height;
			m_vd.SampleDuration = FrameRateToDuration(m_vd.Numerator, m_vd.Denominator);
		}
		else if(majorType == MFMediaType_Audio)
		{
			m_vd.HasAudio = true;

			if(pFinalType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &m_vd.ASampleRate) != S_OK)
			{
				return;
			}

			if(pFinalType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &m_vd.AChannelCount) != S_OK)
			{
				return;
			}

			if(pFinalType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &m_vd.ABitsPerSample) != S_OK)
			{
				return;
			}
		}
	}

	//Load first frame
	if(!loadNextFrame())
	{
		return;
	}	

	D3D11_TEXTURE2D_DESC decs;
	m_pTexture->GetDesc(&decs);

	// Create buffer for shader
	ComPtr<ID3D11Device> pDevice;
	D3D11_BUFFER_DESC cbDesc;

	cbDesc.ByteWidth = sizeof(SVideoShaderData);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	RET_IFFAIL(pDXManager->LockDevice(deviceHandle, IID_PPV_ARGS(&pDevice), TRUE))
	RET_IFFAIL(pDevice->CreateBuffer(&cbDesc, NULL, &m_pVDBuffer))
	pDevice.Reset();
	pDXManager->UnlockDevice(deviceHandle, TRUE);

	//Done
	m_Initialized = true;
}

CSource::~CSource()
{

}

bool CSource::LoadNextFrame()
{
	if(!m_Initialized)
	{
		return false;
	}

	return loadNextFrame();
}

bool CSource::loadNextFrame()
{
	m_pTexture = nullptr;

	ComPtr<IMFSample> pSample;
	ComPtr<IMFMediaBuffer> pBuffer;
	ComPtr<IMFDXGIBuffer> pDXGIBuffer;

	DWORD ActualStreamIndex = 0;
	DWORD StreamFlags = 0;
	LONGLONG SampleTimestamp = 0;

	if(m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample) != S_OK)
	{
		return false;
	}

	if(pSample == nullptr)
	{
		return false;
	}

	if(pSample->GetBufferByIndex(0, &pBuffer) != S_OK)
	{
		return false;
	}

	if(pBuffer.As(&pDXGIBuffer) == S_OK)
	{
		if(pDXGIBuffer->GetResource(IID_PPV_ARGS(&m_pTexture)) == S_OK)
		{
			pDXGIBuffer->GetSubresourceIndex(&m_TexIndex);
		}
	}

	return m_pTexture.Get() != nullptr;
}

bool CSource::InitRuntimeVariables(ID3D11DeviceContext* pDXContext, const SVideoData* vd, LONGLONG beginningTime)
{
	if(m_vd.Width >= vd->Width)
	{
		if(((float)vd->Width / m_vd.Width * m_vd.Height) > vd->Height)
		{
			AdjustByHeight(vd);
		}
		else
		{
			AdjustByWidth(vd);
		}
	}
	else
	{
		AdjustByHeight(vd);
	}
	
	m_vsd.fading = 1.0f;

	StartTime = beginningTime;
	EndTime = StartTime + m_vd.Duration;
	IntroTime = StartTime + m_Intro.Duration;
	OutroTime = EndTime - m_Outro.Duration;	

	RETFALSE_IFFALSE(UpdateBuffers(pDXContext))

	return true;
}

bool CSource::UpdateBuffers(ID3D11DeviceContext* pDXContext)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	RETFALSE_IFFAIL(pDXContext->Map(m_pVDBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
	memcpy(mappedResource.pData, &m_vsd, sizeof(m_vsd));
	pDXContext->Unmap(m_pVDBuffer.Get(), 0);

	return true;
}

void CSource::SetBuffers(ID3D11DeviceContext* pDXContext)
{
	pDXContext->PSSetConstantBuffers(0, 1, m_pVDBuffer.GetAddressOf());
}

void CSource::AdjustByHeight(const SVideoData* vd)
{
	float width, height;
	
	if(m_vd.Height >= vd->Height)
	{
		height = (float) vd->Height;
		width = height / m_vd.Height * m_vd.Width;
	}
	else
	{
		height = (float) m_vd.Height;
		width = (float) m_vd.Width;
	}

	m_vsd.scale[0] = width / vd->Width;
	m_vsd.scale[1] = height / vd->Height;
	m_vsd.offset[0] = ((float)vd->Width - width) / vd->Width / 2.0f;
	m_vsd.offset[1] = ((float)vd->Height - height) / vd->Height / 2.0f;
}

void CSource::AdjustByWidth(const SVideoData* vd)
{
	float width, height;

	if(m_vd.Width >= vd->Width)
	{
		width = (float) vd->Width;
		height = width / m_vd.Width * m_vd.Height;
	}
	else
	{
		height = (float) m_vd.Height;
		width = (float) m_vd.Width;
	}

	m_vsd.scale[0] = width / vd->Width;
	m_vsd.scale[1] = height / vd->Height;
	m_vsd.offset[0] = ((float)vd->Width - width) / vd->Width / 2.0f;
	m_vsd.offset[1] = ((float)vd->Height - height) / vd->Height / 2.0f;
}

LONGLONG CSource::FrameRateToDuration(UINT32 Numerator, UINT32 Denominator)
{
	if(Numerator == 60000 && Denominator == 1001)
	{
		return 166833;
	}
	else if(Numerator == 30000 && Denominator == 1001)
	{
		return 333667;
	}
	else if(Numerator == 24000 && Denominator == 1001)
	{
		return 417188;
	}
	else if(Numerator == 60 && Denominator == 1)
	{
		return 166667;
	}
	else if(Numerator == 30 && Denominator == 1)
	{
		return 333333;
	}
	else if(Numerator == 50 && Denominator == 1)
	{
		return 200000;
	}
	else if(Numerator == 25 && Denominator == 1)
	{
		return 400000;
	}
	else if(Numerator == 24 && Denominator == 1)
	{
		return 416667;
	}
	//else

	return (LONGLONG) Denominator * 10000000 / Numerator;
}