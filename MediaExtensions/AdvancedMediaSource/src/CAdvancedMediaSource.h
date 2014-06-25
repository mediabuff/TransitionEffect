#pragma once

#include <vector>
#include "pch.h"
#include "CritSec.h"
#include "CSource.h"

namespace AdvancedMediaSource
{
	public ref class CAdvancedMediaSource sealed
	{
	public:

		CAdvancedMediaSource();
		virtual ~CAdvancedMediaSource();

		bool AddVideo(Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect);
		void GetVideoData(SVideoData* vd) { *vd = m_vd; }
		bool IsInitialized() { return m_Initialized; }

		bool OnStart(Windows::Media::Core::VideoStreamDescriptor^ videoDesc);
		void GenerateVideoSample(Windows::Media::Core::MediaStreamSourceSampleRequest^ request);
		void GenerateAudioSample(Windows::Media::Core::MediaStreamSourceSampleRequest^ request);

	private:

		bool InitResources();
		bool DXLock();
		void DXUnlock();
		bool CreateInputView(ID3D11Texture2D* pTexture, UINT Index, ID3D11ShaderResourceView** ppView);
		bool CreateOutputView(IMFSample* pSample, ID3D11RenderTargetView** ppView);
		bool Draw(ID3D11ShaderResourceView* pInputView, ID3D11RenderTargetView* pOutputView);
		
	private:

		std::vector<CSourcePtr> m_Sources;
		int m_CurrentVideo;

		CritSec m_critSec;

		// DirectX manager
		ComPtr<IMFDXGIDeviceManager> m_pDXManager;		
		UINT m_ResetToken;

		// DX Device
		ComPtr<ID3D11Device> m_pDX11Device;
		HANDLE m_DeviceHandle;
		ComPtr<ID3D11DeviceContext> m_pContext;
		ComPtr<ID3D11SamplerState> m_pSampleStateLinear;
		ComPtr<ID3D11Buffer> m_pScreenQuadVB;
		ComPtr<ID3D11InputLayout> m_pQuadLayout;
		ComPtr<ID3D11VertexShader> m_pVertexShader;
		ComPtr<ID3D11PixelShader> m_pPixelShader;

		// Runtime draw variables
		ComPtr<ID3D11Device> m_pDevice;
		ComPtr<ID3D11DeviceContext> m_pImmediateContext;
		ID3D11Buffer* m_pBuffers[1];
		UINT m_vbStrides;
		UINT m_vbOffsets;
		ID3D11SamplerState* m_pSamplers[1];
		D3D11_VIEWPORT m_Viewport;

		SVideoData m_vd;
		ComPtr<IMFVideoSampleAllocator> m_pVideoSamplesAllocator;		
		LONGLONG m_VideoTimestamp;

		bool m_Initialized;		
	};
}
