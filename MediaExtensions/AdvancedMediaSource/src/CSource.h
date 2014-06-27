#pragma once

#include <memory>
#include "pch.h"
#include "SVideoData.h"
#include "SShaderFrameData.h"
#include "VideoTypes.h"
#include "Quad.h"

namespace AdvancedMediaSource
{
	class CSource
	{
	public:

		CSource(IMFDXGIDeviceManager* pDXManager, Platform::String^ url, EIntroType intro, UINT32 introDuration, EOutroType outro, UINT32 outroDuration, EVideoEffect videoEffect);
		~CSource();

		bool LoadNextFrame();
		void GetAudioSample(IMFSample** ppSample);
		bool InitRuntimeVariables(ID3D11DeviceContext* pDXContext, const SVideoData* vd, LONGLONG beginningTime);
		bool UpdateBuffers(ID3D11DeviceContext* pDXContext);
		void SetBuffers(ID3D11DeviceContext* pDXContext);

		void SetFading(float fading) { /*m_vsd.fading = fading;*/ }
		IMFSample* GetSample() { return m_pSample.Get(); }
		void GetVideoData(SVideoData* vd) { *vd = m_vd; }
		LONGLONG GetDuration() { return m_vd.Duration; }
		EIntroType GetIntroType() { return m_Intro.Type; }
		LONGLONG GetIntroDuration() { return m_Intro.Duration; }
		EOutroType GetOutroType() { return m_Outro.Type; }
		LONGLONG GetOutroDuration() { return m_Outro.Duration; }
		
		bool IsInitialized() { return m_Initialized; }
		bool HasFrameLoaded() { return m_pSample.Get() != nullptr; }

	public:

		//runtime variables assigned outside this class
		LONGLONG StartTime, IntroTime, OutroTime, EndTime;

	private:

		bool loadNextFrame();
		void AdjustByHeight(const SVideoData* vd);
		void AdjustByWidth(const SVideoData* vd);
		void SetQuad(float dx, float dy);

		static LONGLONG FrameRateToDuration(UINT32 Numerator, UINT32 Denominator);

	private:

		ComPtr<IMFSourceReader> m_pSourceReader;
		ComPtr<IMFSample> m_pSample;
		ComPtr<ID3D11Buffer> m_pFrameDataCB;
		ComPtr<ID3D11Buffer> m_pFrameQuadVB;
		SVideoData m_vd;
		SShaderFrameData m_sfd;
		SQuad m_FrameQuad;
		UINT m_vbStrides, m_vbOffsets;

		EVideoEffect m_VideoEffect;
		SIntro m_Intro; 
		SOutro m_Outro;

		bool m_Initialized;
		bool m_FrameQuadBufferValid;
	};

	typedef std::shared_ptr<CSource> CSourcePtr;
}
