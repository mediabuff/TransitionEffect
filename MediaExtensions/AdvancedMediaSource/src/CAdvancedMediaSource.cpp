#include "pch.h"
#include "CAdvancedMediaSource.h"
#include "helpers.h"

using namespace AdvancedMediaSource;
using namespace Platform;
using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Foundation;

CAdvancedMediaSource::CAdvancedMediaSource()
{
	m_Initialized = false;
	m_ResetToken = 0;

	if(MFStartup(MF_VERSION) != S_OK)
	{
		return;
	}
	
	if(MFCreateDXGIDeviceManager(&m_ResetToken, &m_pDXManager) != S_OK)
	{
		return;
	}

	D3D_FEATURE_LEVEL maxSupportedLevelByDevice = D3D_FEATURE_LEVEL_10_0;
	D3D_FEATURE_LEVEL rgFeatureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	HRESULT hr;

	if((hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, rgFeatureLevels, ARRAYSIZE(rgFeatureLevels), D3D11_SDK_VERSION, &m_pDX11Device, &maxSupportedLevelByDevice, NULL)) != S_OK)
	{
		return;
	}		

	if(m_pDXManager->ResetDevice(m_pDX11Device.Get(), m_ResetToken) != S_OK)
	{
		return;
	}

	if(m_pDXManager->OpenDeviceHandle(&m_DeviceHandle) != S_OK)
	{
		return;
	}

	if(!InitResources())
	{
		return;
	}


	/*ComPtr<IMFMediaType> pType;

	if(MFCreateMediaType(&pType) != S_OK)
	{
	return;
	}

	pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

	HRESULT hr;*/

	m_Initialized = true;
}

CAdvancedMediaSource::~CAdvancedMediaSource()
{
	m_Sources.clear();

	if(m_pVideoSamplesAllocator != nullptr)
	{
		m_pVideoSamplesAllocator->UninitializeSampleAllocator();
	}

	m_pDXManager->CloseDeviceHandle(m_DeviceHandle);

	MFShutdown();
}

bool CAdvancedMediaSource::AddVideo(Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect)
{
	AutoLock lock(m_critSec);
	SVideoData vd;
	
	CSourcePtr pV(new CSource(m_pDXManager.Get(), m_DeviceHandle, url, intro, introDuration, outro, outroDuration, videoEffect));

	if(!pV->IsInitialized())
	{
		return false;
	}

	pV->GetVideoData(&vd);

	if(m_Sources.size() == 0)
	{
		m_vd = vd;
	}
	else if((vd.Width * vd.Height) < (m_vd.Width * m_vd.Height))
	{
		m_vd = vd;
	}

	m_Sources.push_back(pV);

	return true;
}

bool CAdvancedMediaSource::OnStart(Windows::Media::Core::VideoStreamDescriptor^ videoDesc)
{
	if(!m_Initialized)
	{
		return false;
	}

	m_Initialized = false; // "OnStart" is a final part of initialization, so we reset m_Initialized

	ComPtr<IMFMediaType> pOutputType;

	RETFALSE_IFFAIL(MFCreateMediaType(&pOutputType))

	RETFALSE_IFFAIL(pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))
	RETFALSE_IFFAIL(pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32))// MFVideoFormat_NV12))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_COMPRESSED, FALSE))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_SAMPLE_SIZE, m_vd.SampleSize))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_DEFAULT_STRIDE, m_vd.Stride))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_DRM_FLAGS, MFVideoDRMFlag_None))
	RETFALSE_IFFAIL(MFSetAttributeRatio(pOutputType.Get(), MF_MT_FRAME_RATE, m_vd.Numerator, m_vd.Denominator))
	RETFALSE_IFFAIL(MFSetAttributeSize(pOutputType.Get(), MF_MT_FRAME_SIZE, m_vd.Width, m_vd.Height))
	RETFALSE_IFFAIL(pOutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive))
	RETFALSE_IFFAIL(MFSetAttributeRatio(pOutputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1))

	RETFALSE_IFFAIL(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&m_pVideoSamplesAllocator)))
	RETFALSE_IFFAIL(m_pVideoSamplesAllocator->SetDirectXManager(m_pDXManager.Get()))
	RETFALSE_IFFAIL(m_pVideoSamplesAllocator->InitializeSampleAllocator(60, pOutputType.Get()))


	//test
	//ComPtr<IMFSample> pSample;
	//ComPtr<IMFMediaBuffer> pBuffer;
	//ComPtr<ID3D11Texture2D> m_pTexture;
	//ComPtr<IMFDXGIBuffer> pDXGIBuffer;
	//UINT m_TexIndex;
	//RETFALSE_IFFAIL(m_pVideoSamplesAllocator->AllocateSample(&pSample))
	//RETFALSE_IFFAIL(pSample->GetBufferByIndex(0, &pBuffer))
	//if(pBuffer.As(&pDXGIBuffer) == S_OK)
	//{
	//	if(pDXGIBuffer->GetResource(IID_PPV_ARGS(&m_pTexture)) == S_OK)
	//	{
	//		pDXGIBuffer->GetSubresourceIndex(&m_TexIndex);
	//	}
	//}

	//D3D11_TEXTURE2D_DESC decs;
	//m_pTexture->GetDesc(&decs);

	RETFALSE_IFFALSE(DXLock())

	LONGLONG tmpTime = 0;
	
	for(int i = 0; i < (int)m_Sources.size(); i++)
	{
		m_Sources[i]->InitRuntimeVariables(m_pImmediateContext.Get(), &m_vd, tmpTime);
		
		tmpTime += m_Sources[i]->GetDuration();
	}

	DXUnlock();

	// -----
	m_Viewport.Width = (float) m_vd.Width;
	m_Viewport.Height = (float) m_vd.Height;
	
	
	m_Initialized = true;
	return true;

	// throw Platform::Exception::CreateException(E_INVALIDARG, L"Unable to initialize resources for the sample generator.");
}

void CAdvancedMediaSource::GenerateVideoSample(Windows::Media::Core::MediaStreamSourceSampleRequest^ request)
{
	ComPtr<IMFMediaStreamSourceSampleRequest> pRequest;
	VideoEncodingProperties^ pEncodingProperties;
	HRESULT hr = (request != nullptr) ? S_OK : E_POINTER;
	UINT32 ui32Height = 0;
	UINT32 ui32Width = 0;
	ULONGLONG ulTimeSpan = 0;
	
	RET_IFFAIL(reinterpret_cast<IInspectable*>(request)->QueryInterface(IID_PPV_ARGS(&pRequest)))

	AutoLock lock(m_critSec);

	if(!m_Initialized)
	{
		return;
	}

	if(m_CurrentVideo < (int) m_Sources.size())
	{
		if(m_Sources[m_CurrentVideo]->HasFrameLoaded())
		{		
			ComPtr<ID3D11ShaderResourceView> pInputView;
			ComPtr<ID3D11RenderTargetView> pOutputView;
			ComPtr<IMFSample> pSample;

			RET_IFFAIL(m_pVideoSamplesAllocator->AllocateSample(&pSample));
			RET_IFFALSE(DXLock())
			RET_IFFALSE(CreateOutputView(pSample.Get(), &pOutputView))
			RET_IFFALSE(CreateInputView(m_Sources[m_CurrentVideo]->GetTexture(), m_Sources[m_CurrentVideo]->GetIndex(), &pInputView))

			if(m_VideoTimestamp < m_Sources[m_CurrentVideo]->IntroTime)
			{
				if(m_Sources[m_CurrentVideo]->GetIntroType() == EIntroType::FadeIn)
				{
					m_Sources[m_CurrentVideo]->SetFading(math_clamp(0.0f, (float)(m_VideoTimestamp - m_Sources[m_CurrentVideo]->StartTime) / m_Sources[m_CurrentVideo]->GetIntroDuration(), 1.0f));
				}
			}
			else if(m_VideoTimestamp < m_Sources[m_CurrentVideo]->OutroTime)
			{
				//Draw()
			}
			else
			{
				if(m_Sources[m_CurrentVideo]->GetOutroType() == EOutroType::FadeOut)
				{
					m_Sources[m_CurrentVideo]->SetFading(math_clamp(0.0f, (float)(m_Sources[m_CurrentVideo]->EndTime - m_VideoTimestamp) / m_Sources[m_CurrentVideo]->GetOutroDuration(), 1.0f));
				}
			}

			m_Sources[m_CurrentVideo]->UpdateBuffers(m_pImmediateContext.Get());
			m_Sources[m_CurrentVideo]->SetBuffers(m_pImmediateContext.Get());
				
						
			Draw(pInputView.Get(), pOutputView.Get());

			DXUnlock();

			pSample->SetSampleDuration(m_vd.SampleDuration);
			pSample->SetSampleTime(m_VideoTimestamp);			
			RET_IFFAIL(pRequest->SetSample(pSample.Get()))
			m_VideoTimestamp += m_vd.SampleDuration;

			if(!m_Sources[m_CurrentVideo]->LoadNextFrame())
			{
				m_CurrentVideo++; // next video must have first frame to be already loaded (while creation)
			}
		}
	}

	//if(requestedSampleType == RequestedSampleType::AUDIO_SAMPLE_REQUESTED)
	//{
	//	ComPtr<IMFSample> pSample0;
	//	DWORD ActualStreamIndex;
	//	DWORD StreamFlags = 100;
	//	LONGLONG SampleTimestamp;

	//	if(m_Sources[0]->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample0) != S_OK)
	//	{
	//		return;
	//	}

	//	if(pSample0.Get() != nullptr)
	//		if(pRequest->SetSample(pSample0.Get()) != S_OK)
	//		{
	//		return;
	//		}

	//	return;

	//}










	//if(SUCCEEDED(hr))
	//{
	//	VideoStreamDescriptor^ pVideoStreamDescriptor = dynamic_cast<VideoStreamDescriptor^>(request->StreamDescriptor);

	//	if(pVideoStreamDescriptor != nullptr)
	//	{
	//		pEncodingProperties = pVideoStreamDescriptor->EncodingProperties;
	//	}
	//	else
	//	{
	//		return;//throw Platform::Exception::CreateException(E_INVALIDARG, L"Media Request is not for an video sample.");
	//	}
	//}

	//if(SUCCEEDED(hr))
	//{
	//	ui32Height = pEncodingProperties->Height;
	//	ui32Width = pEncodingProperties->Width;

	//	MediaRatio^ spRatio = pEncodingProperties->FrameRate;
	//	if(SUCCEEDED(hr))
	//	{
	//		UINT32 ui32Numerator = spRatio->Numerator;
	//		UINT32 ui32Denominator = spRatio->Denominator;
	//		if(ui32Numerator != 0)
	//		{
	//			ulTimeSpan = ((ULONGLONG)ui32Denominator) * 10000000 / ui32Numerator;
	//		}
	//		else
	//		{
	//			hr = E_INVALIDARG;
	//		}
	//	}
	//}

	//if(SUCCEEDED(hr))
	//{
	//	ComPtr<IMFMediaBuffer> pBuffer;
	//	ComPtr<IMFSample> pSample;

	//	hr = m_pVideoSamplesAllocator->AllocateSample(&pSample);

	//	if(SUCCEEDED(hr))
	//	{
	//		hr = pSample->SetSampleDuration(ulTimeSpan);
	//	}

	//	if(SUCCEEDED(hr))
	//	{
	//		hr = pSample->SetSampleTime((LONGLONG)m_VideoTimestamp);
	//	}

	//	if(SUCCEEDED(hr))
	//	{
	//		m_VideoTimestamp += ulTimeSpan;
	//	}

	//	hr = pRequest->SetSample(pSample.Get());
	//}














	//	if(SUCCEEDED(hr))
	//	{

	//		/*if((hr = pSample->GetBufferByIndex(0, &pBuffer)) != S_OK)
	//		{
	//		return;
	//		}*/

	//		//hr = GenerateCubeFrame(ui32Width, ui32Height, _iVideoFrameNumber, spBuffer.Get());

	//		BYTE* pBits = 0;
	//		BYTE* pBits0 = 0;
	//		BYTE* pBits1 = 0;
	//		DWORD ml = 0;

	//		/*if(pBuffer->Lock(&pBits, &ml, NULL) != S_OK)
	//		{
	//		return;
	//		}/**/

	//		ComPtr<IMFSample> pSample0;
	//		ComPtr<IMFSample> pSample1;
	//		ComPtr<IMFMediaBuffer> pBuffer0;
	//		ComPtr<IMFMediaBuffer> pBuffer1;
	//		DWORD ActualStreamIndex;
	//		DWORD StreamFlags = 100;
	//		LONGLONG SampleTimestamp;


	//		if(m_Sources[0]->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample0) != S_OK)
	//		{
	//			return;
	//		}/**/

	//		if(pSample0.Get() != nullptr)
	//			if(pRequest->SetSample(pSample0.Get()) != S_OK)
	//			{
	//			return;
	//			}

	//		/*if(pSample0.Get() != nullptr)
	//		{
	//		StreamFlags = MF_SOURCE_READER_CONTROLF_DRAIN;

	//		pSample0->GetBufferByIndex(0, &pBuffer0);
	//		pBuffer0->Lock(&pBits0, NULL, NULL);
	//		CopyMemory(pBits, pBits0, 1280 * (720 + 720 / 2));
	//		pBuffer0->Unlock();
	//		}
	//		else
	//		{
	//		return;
	//		}

	//		pSample0.Reset();*/

	//		/*if(m_Sources[0]->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample0) != S_OK)
	//		{
	//		return;
	//		}/**/
	//		//pSample0.Reset();

	//		//pSample0.Get()->Release();

	//		/*if(pSample0.Get() != nullptr)
	//		{
	//		StreamFlags = MF_SOURCE_READER_CONTROLF_DRAIN;

	//		pSample0->GetBufferByIndex(0, &pBuffer0);
	//		pBuffer0->Lock(&pBits0, NULL, NULL);
	//		CopyMemory(pBits, pBits0, 1280 * (720 + 720 / 2));
	//		pBuffer0->Unlock();
	//		}
	//		else
	//		{
	//		StreamFlags = 0;
	//		}/**/

	//		/*static ULONGLONG rt = 30000000;
	//		if(pSample0.Get() != nullptr && m_VideoTimestamp > rt)
	//		{
	//		do
	//		{
	//		if(m_Sources[0]->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, MF_SOURCE_READER_CONTROLF_DRAIN, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample0) != S_OK)
	//		{
	//		return;
	//		}

	//		if(pSample0.Get() != nullptr)
	//		{
	//		StreamFlags = MF_SOURCE_READER_CONTROLF_DRAIN;
	//		}

	//		} while(pSample0.Get() != nullptr);

	//		rt += 30000000;
	//		}/**/


	//		/*if((hr = m_Sources[1]->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample1)) != S_OK)
	//		{
	//		return;
	//		}*/

	//		/*if(pSample0->GetBufferByIndex(0, &pBuffer0) != S_OK)
	//		{
	//		return;
	//		}*/

	//		/*if(pSample1->GetBufferByIndex(0, &pBuffer1) != S_OK)
	//		{
	//		return;
	//		}*/

	//		/*if((hr = pBuffer0->Lock(&pBits0, NULL, NULL)) != S_OK)
	//		{
	//		return;
	//		}*/

	//		/*if((hr = pBuffer1->Lock(&pBits1, NULL, NULL)) != S_OK)
	//		{
	//		return;
	//		}*/

	//		//CopyMemory(pBits, pBits0, 1280 * (720 + 720 / 2));
	//		//FillMemory(pBits, 1382400, 200);

	//		/*static int offset = 0, ddd = 100000;
	//		offset += ulTimeSpan;

	//		if(offset >= ui32Width * ddd) { offset = ui32Width * ddd; }

	//		for(int h = 0; h < ui32Height + ui32Height / 2; h++)
	//		{
	//		CopyMemory(pBits + ui32Width * h, pBits0 + ui32Width * h + offset / ddd, ui32Width - offset / ddd);
	//		CopyMemory(pBits + ui32Width * h + ui32Width - offset / ddd, pBits1 + ui32Width * h, offset / ddd);
	//		}*/

	//		//pBuffer1->Unlock();
	//		//pBuffer0->Unlock();			

	//		/*if(pBuffer->Unlock() != S_OK)
	//		{
	//		return;
	//		}

	//		pBuffer.Reset();
	//		pBuffer0.Reset();
	//		pBuffer1.Reset();
	//		pSample0.Reset();
	//		pSample1.Reset();*/
	//	}

	//	/*if(SUCCEEDED(hr))
	//	{
	//	hr = pRequest->SetSample(pSample.Get());
	//	pSample.Reset();
	//	}*/

	//	

	//	if(hr != S_OK)
	//	{
	//		return;
	//	}
	//}
}

void CAdvancedMediaSource::GenerateAudioSample(Windows::Media::Core::MediaStreamSourceSampleRequest^ request)
{

}

struct ScreenVertex
{
	FLOAT pos[4];
	FLOAT tex[2];
};

bool CAdvancedMediaSource::InitResources()
{
	// Fill Screen Quad
	D3D11_BUFFER_DESC VBdesc;

	// 2 vertices (Triangles), covering the whole Viewport, with the input video mapped as a texture
	const ScreenVertex svDefault[4] = 
	{
		//   x      y     z     w         u     v
		{ { -1.0f, 1.0f, 0.5f, 1.0f },  { 0.0f, 0.0f } }, // 0
		{ { 1.0f, 1.0f, 0.5f, 1.0f },   { 1.0f, 0.0f } }, // 1
		{ { -1.0f, -1.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } }, // 2
		{ { 1.0f, -1.0f, 0.5f, 1.0f },  { 1.0f, 1.0f } }  // 3
	};

	ZeroMemory(&VBdesc, sizeof(VBdesc));

	VBdesc.Usage = D3D11_USAGE_DYNAMIC;
	VBdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	VBdesc.MiscFlags = 0;
	VBdesc.ByteWidth = sizeof(svDefault);

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = svDefault;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	if(m_pDX11Device->CreateBuffer(&VBdesc, &InitData, &m_pScreenQuadVB) != S_OK)
	{
		return false;
	}


	// Pixel shader
	static
	#include "PixelShader.h"

	if(m_pDX11Device->CreatePixelShader(g_psshader, sizeof(g_psshader), NULL, &m_pPixelShader) != S_OK)
	{
		return false;
	}


	// Vertex Shader
	static
	#include "VertexShader.h"
		
	const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
		
	if(m_pDX11Device->CreateVertexShader(g_vsshader, sizeof(g_vsshader), NULL, &m_pVertexShader) != S_OK)
	{
		return false;
	}

	if(m_pDX11Device->CreateInputLayout(quadlayout, 2, g_vsshader, sizeof(g_vsshader), &m_pQuadLayout) != S_OK)
	{
		return false;
	}

	// Sampler
	D3D11_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));

	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if(m_pDX11Device->CreateSamplerState(&SamplerDesc, &m_pSampleStateLinear) != S_OK)
	{
		return false;
	}

	//--------
	m_pBuffers[0] = m_pScreenQuadVB.Get();
	m_vbStrides = sizeof(ScreenVertex);
	m_vbOffsets = 0;
	m_pSamplers[0] = m_pSampleStateLinear.Get();

	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;

	m_CurrentVideo = 0;
	m_VideoTimestamp = 0;

	return true;	
}

bool CAdvancedMediaSource::CreateInputView(ID3D11Texture2D* pTexture, UINT Index, ID3D11ShaderResourceView** ppView)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;

	ZeroMemory(&viewDesc, sizeof(viewDesc));
	pTexture->GetDesc(&textureDesc);
	viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;// DXGI_FORMAT_NV12;

	if(textureDesc.ArraySize > 1)
	{
		// Texture array, use the subresource index
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		viewDesc.Texture2DArray.FirstArraySlice = Index;
		viewDesc.Texture2DArray.ArraySize = 1;
		viewDesc.Texture2DArray.MipLevels = 1;
		viewDesc.Texture2DArray.MostDetailedMip = 0;
	}
	else
	{
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = 1;
	}

	HRESULT hr = m_pDevice->CreateShaderResourceView(pTexture, &viewDesc, ppView);

	return hr == S_OK;
}

bool CAdvancedMediaSource::CreateOutputView(IMFSample* pSample, ID3D11RenderTargetView** ppView)
{
	assert(pSample != nullptr);

	ComPtr<ID3D11Texture2D> pTexture;	
	ComPtr<IMFMediaBuffer> pBuffer;
	ComPtr<IMFDXGIBuffer> pDXGIBuffer;
	UINT Index = 0;

	RETFALSE_IFFAIL(pSample->GetBufferByIndex(0, &pBuffer))
	RETFALSE_IFFAIL(pBuffer.As(&pDXGIBuffer))
	RETFALSE_IFFAIL(pDXGIBuffer->GetResource(IID_PPV_ARGS(&pTexture)))
	RETFALSE_IFFAIL(pDXGIBuffer->GetSubresourceIndex(&Index))

	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC viewDesc;

	ZeroMemory(&viewDesc, sizeof(viewDesc));
	pTexture->GetDesc(&textureDesc);
	viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;// DXGI_FORMAT_NV12;

	if(textureDesc.ArraySize > 1)
	{
		// Texture array, use the subresource index
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		viewDesc.Texture2DArray.FirstArraySlice = Index;
		viewDesc.Texture2DArray.ArraySize = 1;
		viewDesc.Texture2DArray.MipSlice = 0;
	}
	else
	{
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
	}

	return m_pDevice->CreateRenderTargetView(pTexture.Get(), &viewDesc, ppView) == S_OK;
}

bool CAdvancedMediaSource::Draw(ID3D11ShaderResourceView* pInputView, ID3D11RenderTargetView* pOutputView)
{
	ComPtr<ID3D11RenderTargetView> pOrigRTV;
	ComPtr<ID3D11DepthStencilView> pOrigDSV;
	ComPtr<ID3D11ShaderResourceView> pOrigSRV;
	D3D11_VIEWPORT vpOld;
	UINT nViewPorts = 1;
	const FLOAT ColorBlack[4] = {0.0f, 0.0f, 0.0f, 1.0f};	

	m_pImmediateContext->IASetInputLayout(m_pQuadLayout.Get());
	m_pImmediateContext->IASetVertexBuffers(0, 1, m_pBuffers, &m_vbStrides, &m_vbOffsets);
	m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pImmediateContext->PSSetSamplers(0, 1, m_pSamplers);
	m_pImmediateContext->OMGetRenderTargets(1, &pOrigRTV, &pOrigDSV);
	m_pImmediateContext->OMSetRenderTargets(1, &pOutputView, nullptr);
	m_pImmediateContext->PSGetShaderResources(0, 1, &pOrigSRV);
	m_pImmediateContext->PSSetShaderResources(0, 1, &pInputView);
	m_pImmediateContext->RSGetViewports(&nViewPorts, &vpOld);

	m_pImmediateContext->RSSetViewports(1, &m_Viewport);
	m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	m_pImmediateContext->ClearRenderTargetView(pOutputView, ColorBlack);
	m_pImmediateContext->Draw(4, 0);

	// Restore the Old
	m_pImmediateContext->RSSetViewports(nViewPorts, &vpOld);
	m_pImmediateContext->PSSetShaderResources(0, 1, pOrigSRV.GetAddressOf());
	m_pImmediateContext->OMSetRenderTargets(1, pOrigRTV.GetAddressOf(), pOrigDSV.Get());

	return true;
}

bool CAdvancedMediaSource::DXLock()
{
	RETFALSE_IFFAIL(m_pDXManager->LockDevice(m_DeviceHandle, IID_PPV_ARGS(&m_pDevice), TRUE))
	m_pDevice->GetImmediateContext(&m_pImmediateContext);

	return true;
}

void CAdvancedMediaSource::DXUnlock()
{
	m_pImmediateContext.Reset();
	m_pDevice.Reset();
	m_pDXManager->UnlockDevice(m_DeviceHandle, TRUE);
}