#pragma once

#include <memory>
#include "pch.h"
#include "SVideoData.h"
#include "SVideoShaderData.h"
#include "VideoTypes.h"

namespace AdvancedMediaSource
{
	class CSource
	{
	public:

		CSource(IMFDXGIDeviceManager* pDXManager, HANDLE deviceHandle, Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect);
		~CSource();

		bool LoadNextFrame();
		void GetAudioSample(IMFSample** ppSample);
		bool InitRuntimeVariables(ID3D11DeviceContext* pDXContext, const SVideoData* vd, LONGLONG beginningTime);
		bool UpdateBuffers(ID3D11DeviceContext* pDXContext);
		void SetBuffers(ID3D11DeviceContext* pDXContext);

		void SetFading(float fading) { m_vsd.fading = fading; }
		ID3D11Texture2D* GetTexture() { return m_pTexture.Get(); }		
		UINT GetIndex() { return m_TexIndex; }
		void GetVideoData(SVideoData* vd) { *vd = m_vd; }
		LONGLONG GetDuration() { return m_vd.Duration; }
		EIntroType GetIntroType() { return m_Intro.Type; }
		LONGLONG GetIntroDuration() { return m_Intro.Duration; }
		EOutroType GetOutroType() { return m_Outro.Type; }
		LONGLONG GetOutroDuration() { return m_Outro.Duration; }
		
		bool IsInitialized() { return m_Initialized; }
		bool HasFrameLoaded() { return m_pTexture.Get() != nullptr; }	

	public:

		//runtime variables assigned outside this class
		LONGLONG StartTime, IntroTime, OutroTime, EndTime;

	private:

		bool loadNextFrame();
		void AdjustByHeight(const SVideoData* vd);
		void AdjustByWidth(const SVideoData* vd);
		static LONGLONG FrameRateToDuration(UINT32 Numerator, UINT32 Denominator);

	private:

		ComPtr<IMFSourceReader> m_pSourceReader;
		ComPtr<ID3D11Texture2D> m_pTexture;
		ComPtr<ID3D11Buffer> m_pVDBuffer;
		UINT m_TexIndex;
		SVideoData m_vd;
		SVideoShaderData m_vsd;

		EVideoEffect m_VideoEffect;
		SIntro m_Intro; 
		SOutro m_Outro;

		bool m_Initialized;
	};

	typedef std::shared_ptr<CSource> CSourcePtr;
}
