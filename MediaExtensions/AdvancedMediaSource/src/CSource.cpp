#include "pch.h"
#include "CSource.h"
#include "helpers.h"

using namespace AdvancedMediaSource;

CSource::CSource(IMFDXGIDeviceManager* pDXManager, Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect)
	: m_Initialized(false)
{
	m_Intro.Type = intro;
	m_Intro.Duration = introDuration * 10000;
	m_Outro.Type = outro;
	m_Outro.Duration = outroDuration * 10000;
	m_vd.HasAudio = false;

	HRESULT hr;
	ComPtr<IMFAttributes> m_pAttributes;	

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

	ComPtr<IMFMediaSource> pMediaSource;
	ComPtr<IMFPresentationDescriptor> pPresentationDesc;

	if(m_pSourceReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, GUID_NULL, IID_PPV_ARGS(&pMediaSource)) != S_OK)
	{
		return;
	}

	ComPtr<IMFRateSupport> rr;
	ComPtr<IMFRateControl> rr2;
	float rate = 537.852f;
	hr = pMediaSource.As(&rr);
	hr = rr->GetFastestRate(MFRATE_REVERSE, FALSE, &rate);
	hr = rr->GetFastestRate(MFRATE_FORWARD, FALSE, &rate);

	hr = pMediaSource.As(&rr2);
	//hr = rr2->SetRate(FALSE, 2.0f);

	if(pMediaSource->CreatePresentationDescriptor(&pPresentationDesc) != S_OK)
	{
		return;
	}

	if(pPresentationDesc->GetUINT64(MF_PD_DURATION, (UINT64*)&m_vd.Duration) != S_OK)
	{
		return;
	}

	//PROPVARIANT var;
	//PropVariantInit(&var);
	//var.vt = VT_I8;
	//var.hVal.QuadPart = 100000000; // 10^7 = 1 second.

	//hr = pMediaSource->Start(pPresentationDesc.Get(), NULL, &var);

	//PropVariantClear(&var);


	//Load first frame
	if(!loadNextFrame())
	{
		return;
	}	

	//D3D11_TEXTURE2D_DESC decs;
	//m_pTexture->GetDesc(&decs);
	
	HANDLE deviceHandle;
	ComPtr<ID3D11Device> pDevice;

	RET_IFFAIL(pDXManager->OpenDeviceHandle(&deviceHandle))
	RET_IFFAIL(pDXManager->LockDevice(deviceHandle, IID_PPV_ARGS(&pDevice), TRUE))
	
	// Create buffer for shader
	
	D3D11_BUFFER_DESC cbDesc;

	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.ByteWidth = sizeof(SShaderFrameData);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;// D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
		
	RET_IFFAIL(pDevice->CreateBuffer(&cbDesc, NULL, &m_pFrameDataCB))

	// Fill Screen Quad
	D3D11_BUFFER_DESC VBdesc;

	ZeroMemory(&VBdesc, sizeof(VBdesc));
	VBdesc.Usage = D3D11_USAGE_DYNAMIC;
	VBdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	VBdesc.MiscFlags = 0;
	VBdesc.ByteWidth = sizeof(m_FrameQuad);
	
	m_vbStrides = sizeof(SVertex);
	m_vbOffsets = 0;

	if(pDevice->CreateBuffer(&VBdesc, nullptr, &m_pFrameQuadVB) != S_OK)
	{
		return;
	}
	
	pDevice.Reset();
	pDXManager->UnlockDevice(deviceHandle, FALSE);

	m_FrameQuadBufferValid = false;

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
	DWORD ActualStreamIndex = 0;
	DWORD StreamFlags = 0;
	LONGLONG SampleTimestamp = 0;

	if(m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &m_pSample) != S_OK)
	{
		return false;
	}

	return m_pSample.Get() != nullptr;
}

void CSource::GetAudioSample(IMFSample** ppSample)
{
	if(!m_Initialized)
	{
		return;
	}

	DWORD ActualStreamIndex = 0;
	DWORD StreamFlags = 0;
	LONGLONG SampleTimestamp = 0;

	m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, ppSample);	
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

	// 2 vertices (Triangles), covering the whole Viewport, with the input video mapped as a texture
	//const ScreenVertex svDefault[4] =
	//{
	//	//   x      y     z     w         u     v
	//	{ { -1.0f, 1.0f, 0.5f, 1.0f }, { 0.0f, 0.0f } }, // 0
	//	{ { 1.0f, 1.0f, 0.5f, 1.0f }, { 1.0f, 0.0f } }, // 1
	//	{ { -1.0f, -1.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } }, // 2
	//	{ { 1.0f, -1.0f, 0.5f, 1.0f }, { 1.0f, 1.0f } }  // 3
	//}; asas

	m_FrameQuad.v[0].z = 0.5f;
	m_FrameQuad.v[0].w = 1.0f;
	m_FrameQuad.v[0].u = 0.0f;
	m_FrameQuad.v[0].v = 0.0f;

	m_FrameQuad.v[1].z = 0.5f;
	m_FrameQuad.v[1].w = 1.0f;
	m_FrameQuad.v[1].u = 1.0f;
	m_FrameQuad.v[1].v = 0.0f;

	m_FrameQuad.v[2].z = 0.5f;
	m_FrameQuad.v[2].w = 1.0f;
	m_FrameQuad.v[2].u = 0.0f;
	m_FrameQuad.v[2].v = 1.0f;

	m_FrameQuad.v[3].z = 0.5f;
	m_FrameQuad.v[3].w = 1.0f;
	m_FrameQuad.v[3].u = 1.0f;
	m_FrameQuad.v[3].v = 1.0f;
	
	//m_vsd.fading = 1.0f;

	m_FrameQuadBufferValid = false;

	StartTime = beginningTime;
	EndTime = StartTime + m_vd.Duration;
	IntroTime = StartTime + m_Intro.Duration;
	OutroTime = EndTime - m_Outro.Duration;	

	RETFALSE_IFFALSE(UpdateBuffers(pDXContext))

	return true;
}

bool CSource::UpdateBuffers(ID3D11DeviceContext* pDXContext)
{
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	//RETFALSE_IFFAIL(pDXContext->Map(m_pConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
	//memcpy(mappedResource.pData, &m_vsd, sizeof(m_vsd));
	//pDXContext->Unmap(m_pConstBuffer.Get(), 0);

	//pDXContext->UpdateSubresource(m_pConstBuffer.Get(), 0, nullptr, &m_vsd, 0, 0);
	//pDXContext->UpdateSubresource(m_pFrameQuadVB.Get(), 0, nullptr, &m_FrameQuad, 0, 0);

	if(!m_FrameQuadBufferValid)
	{
		RETFALSE_IFFAIL(pDXContext->Map(m_pFrameQuadVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		memcpy(mappedResource.pData, &m_FrameQuad, sizeof(m_FrameQuad));
		pDXContext->Unmap(m_pFrameQuadVB.Get(), 0);

		m_FrameQuadBufferValid = true;
	}

	return true;
}

void CSource::SetBuffers(ID3D11DeviceContext* pDXContext)
{
	//pDXContext->PSSetConstantBuffers(0, 1, m_pConstBuffer.GetAddressOf());

	pDXContext->IASetVertexBuffers(0, 1, m_pFrameQuadVB.GetAddressOf(), &m_vbStrides, &m_vbOffsets);
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

	SetQuad(((float)vd->Width - width) / vd->Width / 2.0f, ((float)vd->Height - height) / vd->Height / 2.0f);
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

	SetQuad(((float)vd->Width - width) / vd->Width / 2.0f, ((float)vd->Height - height) / vd->Height / 2.0f);
}

void CSource::SetQuad(float dx, float dy)
{
	// 2 vertices (Triangles), covering the whole Viewport, with the input video mapped as a texture
	//const ScreenVertex svDefault[4] =
	//{
	//	//   x      y     z     w         u     v
	//	{ { -1.0f, 1.0f, 0.5f, 1.0f }, { 0.0f, 0.0f } }, // 0
	//	{ { 1.0f, 1.0f, 0.5f, 1.0f }, { 1.0f, 0.0f } }, // 1
	//	{ { -1.0f, -1.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } }, // 2
	//	{ { 1.0f, -1.0f, 0.5f, 1.0f }, { 1.0f, 1.0f } }  // 3
	//}; 

	m_FrameQuad.v[0].x = -1.0f + dx;
	m_FrameQuad.v[0].y = 1.0f - dy;

	m_FrameQuad.v[1].x = 1.0f - dx;
	m_FrameQuad.v[1].y = 1.0f - dy;

	m_FrameQuad.v[2].x = -1.0f + dx;
	m_FrameQuad.v[2].y = -1.0f + dy;

	m_FrameQuad.v[3].x = 1.0f - dx;
	m_FrameQuad.v[3].y = -1.0f + dy;
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