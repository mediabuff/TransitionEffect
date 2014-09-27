#include "pch.h"
#include "CSource.h"

using namespace AdvancedMediaSource;

CSource::CSource(IMFDXGIDeviceManager* pDXManager, IMFMediaType * pTargetVideoType, IMFMediaType * pTargetAudioType,
    Platform::String^ url)
    : m_Initialized(false)
    , m_spMediaType(nullptr)
    , m_pSourceReader(nullptr)
    , m_pSample(nullptr)
{

    HRESULT hr;
    ComPtr<IMFAttributes> m_pAttributes;

    if (MFCreateAttributes(&m_pAttributes, 10) != S_OK)
    {
        return;
    }

    //m_pAttributes->SetUINT32(MF_LOW_LATENCY, 1);
    m_pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    m_pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    //m_pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_TRANSCODE_ONLY_TRANSFORMS, TRUE);
    m_pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, pDXManager);

    if (MFCreateSourceReaderFromURL(url->Data(), m_pAttributes.Get(), &m_pSourceReader) != S_OK)
    {
        return;
    }

    for (DWORD streamId = 0; true; streamId++)
    {
        ComPtr<IMFMediaType> pNativeType;
        ComPtr<IMFMediaType> pType;
        GUID majorType, subtype;

        hr = m_pSourceReader->GetNativeMediaType(streamId, 0, &pNativeType);
        if (hr == MF_E_NO_MORE_TYPES)
        {
            continue;
        }
        else if (hr == MF_E_INVALIDSTREAMNUMBER)
        {
            break;
        }
        else if (FAILED(hr))
        {
            return;
        }

        // Find the major type.
        hr = pNativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        if (FAILED(hr))
        {
            return;
        }

        if (pNativeType->GetGUID(MF_MT_SUBTYPE, &subtype) != S_OK)
        {
            return;
        }

        if (MFCreateMediaType(&pType) != S_OK)
        {
            return;
        }

        if (pType->SetGUID(MF_MT_MAJOR_TYPE, majorType) != S_OK)
        {
            return;
        }

        // Select a subtype.
        if (majorType == MFMediaType_Video)
        {
            subtype = MFVideoFormat_ARGB32;
            //MFVideoFormat_NV12;
        }
        else if (majorType == MFMediaType_Audio)
        {
            subtype = MFAudioFormat_PCM;
        }
        else
        {
            continue; // Unrecognized type. Skip.
        }

        if (pType->SetGUID(MF_MT_SUBTYPE, subtype) != S_OK)
        {
            return;
        }

        if (majorType == MFMediaType_Video && pTargetVideoType != nullptr)
        {
            pTargetVideoType->CopyAllItems(pType.Get());

            // preserve original width and height
            UINT32 ori_width, ori_height;
            if (MFGetAttributeSize(pNativeType.Get(), MF_MT_FRAME_SIZE, &ori_width, &ori_height) != S_OK)
            {
                return;
            }

            if (MFSetAttributeSize(pType.Get(), MF_MT_FRAME_SIZE, ori_width, ori_height) != S_OK)
            {
                return;
            }
        }
        else if (majorType == MFMediaType_Audio && pTargetAudioType != nullptr)
        {
            pTargetAudioType->CopyAllItems(pType.Get());
        }

        hr = m_pSourceReader->SetCurrentMediaType(streamId, NULL, pType.Get());

        if (FAILED(hr))
        {
            return;
        }

        // test final type
        hr = m_pSourceReader->GetCurrentMediaType(streamId, &m_spMediaType);
        if (FAILED(hr))
        {
            return;
        }

        DWORD eFlags = 0;

        if (pType->IsEqual(m_spMediaType.Get(), &eFlags) == E_INVALIDARG)
        {
            return;
        }
        else
        {
            if (!(eFlags & (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES)))
            {
                return;
            }
        }

        if (majorType == MFMediaType_Video)
        {
            if (MFGetAttributeSize(m_spMediaType.Get(), MF_MT_FRAME_SIZE, &m_vd.Width, &m_vd.Height) != S_OK)
            {
                return;
            }

            if (MFGetAttributeRatio(m_spMediaType.Get(), MF_MT_FRAME_RATE, &m_vd.Numerator, &m_vd.Denominator) != S_OK)
            {
                return;
            }

            _DPRINTF((L"m_vd.Width %d", m_vd.Width));
            _DPRINTF((L"m_vd.Height %d", m_vd.Height));
            _DPRINTF((L"m_vd.Numerator %d", m_vd.Numerator));
            _DPRINTF((L"m_vd.Denominator %d", m_vd.Denominator));

            m_vd.Stride = m_vd.Width * 4;
            m_vd.SampleSize = m_vd.Stride * m_vd.Height;
            m_vd.SampleDuration = FrameRateToDuration(m_vd.Numerator, m_vd.Denominator);
        }
        else if (majorType == MFMediaType_Audio)
        {
            m_vd.HasAudio = true;

            if (m_spMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &m_vd.ASampleRate) != S_OK)
            {
                return;
            }

            if (m_spMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &m_vd.AChannelCount) != S_OK)
            {
                return;
            }

            if (m_spMediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &m_vd.ABitsPerSample) != S_OK)
            {
                return;
            }

            _DPRINTF((L"m_vd.ASampleRate %d", m_vd.ASampleRate));
            _DPRINTF((L"m_vd.AChannelCount %d", m_vd.AChannelCount));
            _DPRINTF((L"m_vd.ABitsPerSample %d", m_vd.ABitsPerSample));
        }
    }

    ComPtr<IMFMediaSource> pMediaSource;
    ComPtr<IMFPresentationDescriptor> pPresentationDesc;

    if (m_pSourceReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, GUID_NULL, IID_PPV_ARGS(&pMediaSource)) != S_OK)
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

    if (pMediaSource->CreatePresentationDescriptor(&pPresentationDesc) != S_OK)
    {
        return;
    }

    if (pPresentationDesc->GetUINT64(MF_PD_DURATION, (UINT64*)&m_vd.Duration) != S_OK)
    {
        return;
    }

    //Load first frame
    if (!loadNextFrame())
    {
        return;
    }

    //Done
    m_Initialized = true;
}

CSource::~CSource()
{

}

bool CSource::LoadNextFrame()
{
    if (!m_Initialized)
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

    if (m_pSample != nullptr)
    {
        //m_pSample = nullptr;
        m_pSample.Reset();
    }

    HRESULT hr = m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &m_pSample);

    if (hr != S_OK)
    {
        return false;
    }

    return m_pSample.Get() != nullptr;
}

void CSource::GetAudioSample(IMFSample** ppSample)
{
    if (!m_Initialized)
    {
        return;
    }

    DWORD ActualStreamIndex = 0;
    DWORD StreamFlags = 0;
    LONGLONG SampleTimestamp = 0;

    m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, ppSample);
}

LONGLONG CSource::FrameRateToDuration(UINT32 Numerator, UINT32 Denominator)
{
    if (Numerator == 60000 && Denominator == 1001)
    {
        return 166833;
    }
    else if (Numerator == 30000 && Denominator == 1001)
    {
        return 333667;
    }
    else if (Numerator == 24000 && Denominator == 1001)
    {
        return 417188;
    }
    else if (Numerator == 60 && Denominator == 1)
    {
        return 166667;
    }
    else if (Numerator == 30 && Denominator == 1)
    {
        return 333333;
    }
    else if (Numerator == 50 && Denominator == 1)
    {
        return 200000;
    }
    else if (Numerator == 25 && Denominator == 1)
    {
        return 400000;
    }
    else if (Numerator == 24 && Denominator == 1)
    {
        return 416667;
    }
    //else

    return (LONGLONG)Denominator * 10000000 / Numerator;
}