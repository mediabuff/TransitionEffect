#pragma once

#include <memory>
#include "SVideoData.h"
#include "VideoTypes.h"

namespace AdvancedMediaSource
{
    class CSource
    {
    public:

        CSource(IMFDXGIDeviceManager* pDXManager, IMFMediaType * pTargetVideoType, IMFMediaType * pTargetAudioType,
                Platform::String^ url);
        ~CSource();

        bool LoadNextFrame();
        void GetAudioSample(IMFSample** ppSample);

        IMFSample* GetSample() { return m_pSample.Get(); }
        IMFMediaType* GetMediaType() { return m_spMediaType.Get(); }
        void GetVideoData(SVideoData* vd) { *vd = m_vd; }
        LONGLONG GetDuration() { return m_vd.Duration; }
        
        bool IsInitialized() { return m_Initialized; }
        bool HasFrameLoaded() { return m_pSample.Get() != nullptr; }

    private:

        bool loadNextFrame();
        static LONGLONG FrameRateToDuration(UINT32 Numerator, UINT32 Denominator);

    private:

        ComPtr<IMFSourceReader> m_pSourceReader;
        ComPtr<IMFMediaType> m_spMediaType;
        ComPtr<IMFSample> m_pSample;
        SVideoData m_vd;

        bool m_Initialized;
    };

    typedef std::shared_ptr<CSource> CSourcePtr;
}
