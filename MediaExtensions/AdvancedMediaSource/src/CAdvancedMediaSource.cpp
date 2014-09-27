#include "pch.h"
#include "CAdvancedMediaSource.h"
#include "TextureLock.h"
#include "VideoBufferLock.h"
#include "FrameMixer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace AdvancedMediaSource;
using namespace Platform;
using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Foundation;

using namespace DirectX;

CAdvancedMediaSource::CAdvancedMediaSource()
    : m_hDevice(nullptr)
    , m_spSampleAllocator(nullptr)
    , m_spDeviceManager(nullptr)
    , m_spD3DDevice(nullptr)
    , m_spD3DContext(nullptr)
    , m_ulVideoTimestamp(0)
    , m_ulAudioTimestamp(0)
    , m_iVideoFrameNumber(0)
    , m_SourceInitialized(false)
    , m_EndOfStream(false)
    , m_outputWidth(0)
    , m_outputHeight(0)
    , m_spInBufferTex(nullptr)
    , m_spOutBufferTex(nullptr)
    , m_spOutBufferStage(nullptr)
    , m_spInputType(nullptr)
    , m_spOutputType(nullptr)
    , m_spAudioType(nullptr)
{

}

CAdvancedMediaSource::~CAdvancedMediaSource()
{
    if (m_spDeviceManager.Get() != nullptr && m_hDevice != nullptr)
    {
        m_spDeviceManager->CloseDeviceHandle(m_hDevice);
        m_hDevice = nullptr;
    }

    if (m_spSampleAllocator != nullptr)
    {
        m_spSampleAllocator->UninitializeSampleAllocator();
    }

    m_Sources.clear();
}

void CAdvancedMediaSource::Initialize(MediaStreamSource ^ mss, VideoStreamDescriptor ^ videoDesc, AudioStreamDescriptor ^ audioDesc)
{
    ComPtr<IMFDXGIDeviceManagerSource> spDeviceManagerSource;

    HRESULT hr = reinterpret_cast<IInspectable*>(mss)->QueryInterface(spDeviceManagerSource.GetAddressOf());

    if (m_spDeviceManager == nullptr)
    {
        if (SUCCEEDED(hr))
        {
            hr = spDeviceManagerSource->GetManager(m_spDeviceManager.ReleaseAndGetAddressOf());
            if (FAILED(hr))
            {
                UINT uiToken = 0;
                hr = MFCreateDXGIDeviceManager(&uiToken, m_spDeviceManager.ReleaseAndGetAddressOf());
                if (SUCCEEDED(hr))
                {
                    ComPtr<ID3D11Device> spDevice;
                    D3D_FEATURE_LEVEL maxSupportedLevelByDevice = D3D_FEATURE_LEVEL_9_1;
                    D3D_FEATURE_LEVEL rgFeatureLevels[] = {
                        D3D_FEATURE_LEVEL_11_1,
                        D3D_FEATURE_LEVEL_11_0,
                        D3D_FEATURE_LEVEL_10_1,
                        D3D_FEATURE_LEVEL_10_0,
                        D3D_FEATURE_LEVEL_9_3,
                        D3D_FEATURE_LEVEL_9_2,
                        D3D_FEATURE_LEVEL_9_1
                    };

                    hr = D3D11CreateDevice(nullptr,
                        D3D_DRIVER_TYPE_WARP,
                        nullptr,
                        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                        rgFeatureLevels,
                        ARRAYSIZE(rgFeatureLevels),
                        D3D11_SDK_VERSION,
                        spDevice.ReleaseAndGetAddressOf(),
                        &maxSupportedLevelByDevice,
                        nullptr);

                    if (SUCCEEDED(hr))
                    {
                        hr = m_spDeviceManager->ResetDevice(spDevice.Get(), uiToken);
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = m_spDeviceManager->OpenDeviceHandle(&m_hDevice);
        }
    }

    if (SUCCEEDED(hr) && m_spSampleAllocator == nullptr)
    {
        ComPtr<IMFMediaType> spVideoMT;
        ComPtr<IMFMediaType> spAudioMT;

        if (SUCCEEDED(hr))
        {
            hr = ConvertPropertiesToMediaType(videoDesc->EncodingProperties, spVideoMT.GetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            hr = MFGetAttributeSize(spVideoMT.Get(), MF_MT_FRAME_SIZE, &m_outputWidth, &m_outputHeight);
        }

        if (SUCCEEDED(hr))
        {
            hr = ConvertPropertiesToMediaType(audioDesc->EncodingProperties, spAudioMT.GetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            hr = MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(m_spSampleAllocator.ReleaseAndGetAddressOf()));
        }

        if (SUCCEEDED(hr))
        {
            hr = m_spSampleAllocator->SetDirectXManager(m_spDeviceManager.Get());
        }

        if (SUCCEEDED(hr))
        {
            hr = m_spSampleAllocator->InitializeSampleAllocator(60, spVideoMT.Get());
        }

        m_spOutputType = spVideoMT;
        m_spAudioType = spAudioMT;
    }

    // We reset each time we are initialized
    m_ulVideoTimestamp = 0;
    m_ulAudioTimestamp = 0;
    m_EndOfStream = false;
    
    if (!InitialSourceSteams())
    {
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        throw Platform::Exception::CreateException(hr, L"Unable to initialize resources for the sample generator.");
    }
}

void CAdvancedMediaSource::ResetTimeline()
{
    m_VideoList.clear();
    m_TransitionEffectList.clear();
    m_ulVideoTimestamp = 0;
    m_ulAudioTimestamp = 0;
    m_Sources.clear();
}

void CAdvancedMediaSource::AddVideo(Platform::String^ url)
{
    AutoLock lock(m_critSec);

    m_VideoList.push_back(url);
}

void CAdvancedMediaSource::AddTransitionEffect(TransitionEffectType type, float length_in_second)
{
    AutoLock lock(m_critSec);

    TransitionEffect effect;
    effect.m_effect = type;
    effect.m_length = (ULONGLONG)(length_in_second * 10000000);

    m_TransitionEffectList.push_back(effect);
}

bool CAdvancedMediaSource::InitialSourceSteams()
{
    if (nullptr == m_spDeviceManager)
        return false;

    m_Sources.clear();
    m_transition_manager.CleanTimeLine();
    for (int i = 0; i < (int)m_VideoList.size(); i++)
    {
        CSourcePtr pV(new CSource(m_spDeviceManager.Get(), m_spOutputType.Get(), m_spAudioType.Get(), m_VideoList[i]));

        if (!pV->IsInitialized())
        {
            return false;
        }

        m_Sources.push_back(pV);

        // initial transition manager
        m_transition_manager.AddVideoInfoInOrder(pV->GetDuration());
        if (i < (int)m_TransitionEffectList.size())
        {
            m_transition_manager.AddTransitionBetweenVideos(m_TransitionEffectList[i], i);
        }
    }

    m_transition_manager.CompileTimeLine();

    m_SourceInitialized = false;

    return true;
}

HRESULT CAdvancedMediaSource::AddAttribute(_In_ GUID guidKey, _In_ IPropertyValue ^value, _In_ IMFAttributes *pAttr)
{
    HRESULT hr = (value != nullptr && pAttr != nullptr) ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr))
    {
        PropertyType type = value->Type;
        switch (type)
        {
        case PropertyType::UInt8Array:
        {
            Platform::Array<BYTE>^ arr;
            value->GetUInt8Array(&arr);

            hr = (pAttr->SetBlob(guidKey, arr->Data, arr->Length));
        }
            break;

        case PropertyType::Double:
        {
            hr = (pAttr->SetDouble(guidKey, value->GetDouble()));
        }
            break;

        case PropertyType::Guid:
        {
            hr = (pAttr->SetGUID(guidKey, value->GetGuid()));
        }
            break;

        case PropertyType::String:
        {
            hr = (pAttr->SetString(guidKey, value->GetString()->Data()));
        }
            break;

        case PropertyType::UInt32:
        {
            hr = (pAttr->SetUINT32(guidKey, value->GetUInt32()));
        }
            break;

        case PropertyType::UInt64:
        {
            hr = (pAttr->SetUINT64(guidKey, value->GetUInt64()));
        }
            break;

            // ignore unknown values
        }
    }
    return hr;
}

HRESULT CAdvancedMediaSource::ConvertPropertiesToMediaType(_In_ IMediaEncodingProperties ^mep, _Outptr_ IMFMediaType **ppMT)
{
    HRESULT hr = (mep != nullptr && ppMT != nullptr) ? S_OK : E_INVALIDARG;
    ComPtr<IMFMediaType> spMT;
    if (SUCCEEDED(hr))
    {
        *ppMT = nullptr;
        hr = MFCreateMediaType(&spMT);
    }

    if (SUCCEEDED(hr))
    {
        auto it = mep->Properties->First();

        while (SUCCEEDED(hr) && it->HasCurrent)
        {
            auto currentValue = it->Current;
            hr = AddAttribute(currentValue->Key, safe_cast<IPropertyValue^>(currentValue->Value), spMT.Get());
            it->MoveNext();
        }

        if (SUCCEEDED(hr))
        {
            GUID guiMajorType = safe_cast<IPropertyValue^>(mep->Properties->Lookup(MF_MT_MAJOR_TYPE))->GetGuid();

            if (guiMajorType != MFMediaType_Video && guiMajorType != MFMediaType_Audio)
            {
                hr = E_UNEXPECTED;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppMT = spMT.Detach();
    }

    return hr;
}

bool CAdvancedMediaSource::InitializeSource()
{
    if (m_SourceInitialized)
        return S_OK;

    AutoLock lock(m_critSec);

    ComPtr<IMFDXGIBuffer> spDXGIBuffer;

    HRESULT hr = m_spDeviceManager->LockDevice(m_hDevice, IID_PPV_ARGS(m_spD3DDevice.ReleaseAndGetAddressOf()), TRUE);

    if (SUCCEEDED(hr))
    {
        m_spD3DDevice->GetImmediateContext(m_spD3DContext.ReleaseAndGetAddressOf());
        
        m_frameMixer.Initialize(m_spD3DDevice.Get(), m_outputWidth, m_outputHeight);

        m_spDeviceManager->UnlockDevice(m_hDevice, TRUE);
    }

    m_SourceInitialized = true;

    return true;
}

void CAdvancedMediaSource::GenerateVideoSample(MediaStreamSourceSampleRequest ^ request)
{
    ComPtr<IMFMediaStreamSourceSampleRequest> spRequest;
    VideoEncodingProperties^ spEncodingProperties;
    HRESULT hr = (request != nullptr) ? S_OK : E_POINTER;
    UINT32 ui32Height = 0;
    UINT32 ui32Width = 0;
    ULONGLONG ulTimeSpan = 0;

    if (SUCCEEDED(hr))
    {
        hr = reinterpret_cast<IInspectable*>(request)->QueryInterface(spRequest.ReleaseAndGetAddressOf());
    }

    // Make sure we are on the same device if not then reset the device
    if (SUCCEEDED(hr))
    {
        if (FAILED(m_spDeviceManager->TestDevice(m_hDevice)))
        {
            m_spDeviceManager->CloseDeviceHandle(m_hDevice);
            hr = m_spDeviceManager->OpenDeviceHandle(&m_hDevice);
        }
    }

    if (SUCCEEDED(hr))
    {
        VideoStreamDescriptor^ spVideoStreamDescriptor;
        spVideoStreamDescriptor = dynamic_cast<VideoStreamDescriptor^>(request->StreamDescriptor);
        if (spVideoStreamDescriptor != nullptr)
        {
            spEncodingProperties = spVideoStreamDescriptor->EncodingProperties;
        }
        else
        {
            throw Platform::Exception::CreateException(E_INVALIDARG, L"Media Request is not for an video sample.");
        }
    }

    if (SUCCEEDED(hr))
    {
        ui32Height = spEncodingProperties->Height;
    }

    if (SUCCEEDED(hr))
    {
        ui32Width = spEncodingProperties->Width;
    }

    m_outputWidth = ui32Width;
    m_outputHeight = ui32Height;

    if (SUCCEEDED(hr))
    {
        MediaRatio^ spRatio = spEncodingProperties->FrameRate;
        if (SUCCEEDED(hr))
        {
            UINT32 ui32Numerator = spRatio->Numerator;
            UINT32 ui32Denominator = spRatio->Denominator;
            if (ui32Numerator != 0)
            {
                ulTimeSpan = ((ULONGLONG)ui32Denominator) * 10000000 / ui32Numerator;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        ComPtr<IMFMediaBuffer> spBuffer;
        ComPtr<IMFSample> spSample;
        hr = m_spSampleAllocator->AllocateSample(spSample.GetAddressOf());

        if (!SUCCEEDED(hr))
        {
            _DPRINTF((L"Out of Memory"));
        }

        if (SUCCEEDED(hr))
        {
            hr = spSample->SetSampleDuration(ulTimeSpan);
        }

        if (SUCCEEDED(hr))
        {
            hr = spSample->SetSampleTime((LONGLONG)m_ulVideoTimestamp);
        }

        if (SUCCEEDED(hr))
        {
            hr = spSample->GetBufferByIndex(0, spBuffer.GetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            hr = GenerateFrame2(ui32Width, ui32Height, m_ulVideoTimestamp, spBuffer.Get());
        }

        if (SUCCEEDED(hr))
        {
            if (!m_EndOfStream)
                hr = spRequest->SetSample(spSample.Get());
            else
                request->Sample = nullptr;
        }

        if (SUCCEEDED(hr))
        {
            ++m_iVideoFrameNumber;
            m_ulVideoTimestamp += ulTimeSpan;
        }
    }

    _DPRINTF((L"m_iVideoFrameNumber %d", m_iVideoFrameNumber));

    if (FAILED(hr))
    {
        throw Platform::Exception::CreateException(hr);
    }
}

HRESULT CreateMediaSample(DWORD cbData, IMFSample **ppSample)
{
    HRESULT hr = S_OK;

    IMFSample *pSample = NULL;
    IMFMediaBuffer *pBuffer = NULL;

    hr = MFCreateSample(&pSample);

    if (SUCCEEDED(hr))
    {
        hr = MFCreateMemoryBuffer(cbData, &pBuffer);
    }

    if (SUCCEEDED(hr))
    {
        hr = pSample->AddBuffer(pBuffer);
    }

    if (SUCCEEDED(hr))
    {
        *ppSample = pSample;
        (*ppSample)->AddRef();
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}

void CAdvancedMediaSource::GenerateAudioSample(Windows::Media::Core::MediaStreamSourceSampleRequest^ request)
{
    ComPtr<IMFMediaStreamSourceSampleRequest> pRequest;
    HRESULT hr;

    RET_IFFAIL(reinterpret_cast<IInspectable*>(request)->QueryInterface(IID_PPV_ARGS(&pRequest)))

    AutoLock lock(m_critSec);

    std::vector<int> curr_video_index_list(2);

    bool is_end_of_stream = false;
    m_transition_manager.GetCurrentVideoIndexs(m_ulAudioTimestamp, curr_video_index_list, is_end_of_stream);

    if (is_end_of_stream)
    {
        request->Sample = nullptr;
        return;
    }

    ComPtr<IMFSample> pSample;

    const int number_video = curr_video_index_list.size();
    if (0 == number_video)
    {
        // Add empty sample
        CreateMediaSample(0, &pSample);

        LONGLONG SampleDuration = 333333;
        
        pSample->SetSampleTime(m_ulAudioTimestamp);
        pRequest->SetSample(pSample.Get());
        m_ulAudioTimestamp += SampleDuration;

        return;
    }

    int curr_audio = curr_video_index_list[number_video - 1];

    assert((curr_audio < (int)m_Sources.size()));

    if (curr_audio < (int)m_Sources.size())
    {
        ComPtr<IMFSample> pSample;

        m_Sources[curr_audio]->GetAudioSample(&pSample);

        if(pSample != nullptr)
        {
            LONGLONG SampleDuration = 0;

            pSample->GetSampleDuration(&SampleDuration);
            pSample->SetSampleTime(m_ulAudioTimestamp);
            pRequest->SetSample(pSample.Get());
            m_ulAudioTimestamp += SampleDuration;
        }
        else
        {
            // Add empty sample
            CreateMediaSample(0, &pSample);

            LONGLONG SampleDuration = 333333;

            pSample->SetSampleTime(m_ulAudioTimestamp);
            pRequest->SetSample(pSample.Get());
            m_ulAudioTimestamp += SampleDuration;
        }
    }
    else
    {
        // Add empty sample
        CreateMediaSample(0, &pSample);

        LONGLONG SampleDuration = 333333;

        pSample->SetSampleTime(m_ulAudioTimestamp);
        pRequest->SetSample(pSample.Get());
        m_ulAudioTimestamp += SampleDuration;
    }
}

HRESULT CAdvancedMediaSource::GenerateFrame(UINT32 ui32Width, UINT32 ui32Height, int iFrameRotation, IMFMediaBuffer * pBuffer)
{
    InitializeSource();

    HRESULT hr = S_OK;

    AutoLock lock(m_critSec);

    if(m_CurrentVideo < (int) m_Sources.size())
    {
        if(m_Sources[m_CurrentVideo]->HasFrameLoaded())
        {        
            IMFSample* pInputSample = m_Sources[m_CurrentVideo]->GetSample();

            if (pInputSample != nullptr)
            {
                m_spInputType = m_Sources[m_CurrentVideo]->GetMediaType();

                ComPtr<IMFMediaBuffer> spInput;
                pInputSample->ConvertToContiguousBuffer(&spInput);

                bool fDeviceLocked = false;
                if (m_spDeviceManager != nullptr)
                {
                    ThrowIfError(m_spDeviceManager->LockDevice(m_hDevice, IID_PPV_ARGS(m_spD3DDevice.ReleaseAndGetAddressOf()), TRUE));
                    fDeviceLocked = true;
                }

                m_spD3DDevice->GetImmediateContext(m_spD3DContext.ReleaseAndGetAddressOf());

                SVideoData vd;
                m_Sources[m_CurrentVideo]->GetVideoData(&vd);
                m_frameMixer.SetInputSize(m_spD3DContext.Get(), vd.Width, vd.Height);

                ProcessOutput(spInput.Get(), pBuffer);

                if (fDeviceLocked)
                {
                    ThrowIfError(m_spDeviceManager->UnlockDevice(m_hDevice, FALSE));
                }

                if (!m_Sources[m_CurrentVideo]->LoadNextFrame())
                {
                    m_CurrentVideo++; // next video must have first frame to be already loaded (while creation)
                }
            }
        }
    }
    else
    {
        m_EndOfStream = true;
    }

    return hr;
}

HRESULT CAdvancedMediaSource::GenerateFrame2(UINT32 ui32Width, UINT32 ui32Height, ULONGLONG curr_time_stamp, IMFMediaBuffer * pBuffer)
{
    InitializeSource();

    HRESULT hr = S_OK;

    AutoLock lock(m_critSec);

    std::vector<int> curr_video_index_list(2);

    bool is_end_of_stream = false;
    m_transition_manager.GetCurrentVideoIndexs(curr_time_stamp, curr_video_index_list, is_end_of_stream);

    if (is_end_of_stream)
    {
        m_EndOfStream = true;
        return hr;
    }

    if (0 == curr_video_index_list.size())
    {
        bool fDeviceLocked = false;
        if (m_spDeviceManager != nullptr)
        {
            ThrowIfError(m_spDeviceManager->LockDevice(m_hDevice, IID_PPV_ARGS(m_spD3DDevice.ReleaseAndGetAddressOf()), TRUE));
            fDeviceLocked = true;
        }

        m_spD3DDevice->GetImmediateContext(m_spD3DContext.ReleaseAndGetAddressOf());

        TransitionParameter parameter = m_transition_manager.GetVideoTransitionParameter(curr_time_stamp, EMPTY_VIDEO);
        m_frameMixer.SetTransitionparameter(parameter);
        m_frameMixer.SetInputSize(m_spD3DContext.Get(), m_outputWidth, m_outputHeight);

        ProcessOutput(pBuffer);

        if (fDeviceLocked)
        {
            ThrowIfError(m_spDeviceManager->UnlockDevice(m_hDevice, FALSE));
        }

        return hr;
    }

    for (int i = 0; i < (int)curr_video_index_list.size(); i++)
    {
        int video_index = curr_video_index_list[i];

        assert(video_index < (int)m_Sources.size());

        if (m_Sources[video_index]->HasFrameLoaded())
        {
            IMFSample* pInputSample = m_Sources[video_index]->GetSample();

            if (pInputSample != nullptr)
            {
                m_spInputType = m_Sources[video_index]->GetMediaType();

                ComPtr<IMFMediaBuffer> spInput;
                pInputSample->ConvertToContiguousBuffer(&spInput);

                bool fDeviceLocked = false;
                if (m_spDeviceManager != nullptr)
                {
                    ThrowIfError(m_spDeviceManager->LockDevice(m_hDevice, IID_PPV_ARGS(m_spD3DDevice.ReleaseAndGetAddressOf()), TRUE));
                    fDeviceLocked = true;
                }

                m_spD3DDevice->GetImmediateContext(m_spD3DContext.ReleaseAndGetAddressOf());

                SVideoData vd;
                m_Sources[video_index]->GetVideoData(&vd);

                TransitionParameter parameter = m_transition_manager.GetVideoTransitionParameter(curr_time_stamp, video_index);
                m_frameMixer.SetTransitionparameter(parameter);
                m_frameMixer.SetInputSize(m_spD3DContext.Get(), vd.Width, vd.Height);

                ProcessOutput(spInput.Get(), pBuffer);

                if (fDeviceLocked)
                {
                    ThrowIfError(m_spDeviceManager->UnlockDevice(m_hDevice, FALSE));
                }

                m_Sources[video_index]->LoadNextFrame();
            }
        }
    }

    return hr;
}

// Reads DX buffers from IMFMediaBuffer
ComPtr<ID3D11Texture2D> BufferToDXType(IMFMediaBuffer *pBuffer, _Out_ UINT *uiViewIndex)
{
    ComPtr<IMFDXGIBuffer> spDXGIBuffer;
    ComPtr<ID3D11Texture2D> spTexture;

    if (SUCCEEDED(pBuffer->QueryInterface(IID_PPV_ARGS(&spDXGIBuffer))))
    {
        if (SUCCEEDED(spDXGIBuffer->GetResource(IID_PPV_ARGS(&spTexture))))
        {
            spDXGIBuffer->GetSubresourceIndex(uiViewIndex);
        }
    }

    return spTexture;
}

// Get the default stride for a video format. 
LONG GetDefaultStride(IMFMediaType *pType)
{
    LONG lStride = 0;

    // Try to get the default stride from the media type.
    if (FAILED(pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&lStride)))
    {
        UINT32 width = 0;
        UINT32 height = 0;

        // Get the subtype and the image size.
        ThrowIfError(MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height));
        lStride = width * 4;

        // Set the attribute for later reference.
        (void)pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
    }

    return lStride;
}

// Generate output data.
void CAdvancedMediaSource::ProcessOutput(IMFMediaBuffer *pIn, IMFMediaBuffer *pOut)
{
    ComPtr<ID3D11Texture2D> spInTex;
    ComPtr<ID3D11Texture2D> spOutTex;
    UINT uiInIndex = 0;
    UINT uiOutIndex = 0;

    // Attempt to convert directly to DX textures
    try
    {
        spInTex = BufferToDXType(pIn, &uiInIndex);
    }
    catch (Exception^)
    {
    }
    try
    {
        spOutTex = BufferToDXType(pOut, &uiOutIndex);
    }
    catch (Exception^)
    {
    }

    bool fNativeIn = spInTex != nullptr;
    bool fNativeOut = spOutTex != nullptr;

    // If the input or output textures' device does not match our device
    // we have to move them to our device
    if (fNativeIn)
    {
        ComPtr<ID3D11Device> spDev;
        spInTex->GetDevice(&spDev);
        if (spDev != m_spD3DDevice)
        {
            fNativeIn = false;
        }
    }
    if (fNativeOut)
    {
        ComPtr<ID3D11Device> spDev;
        spOutTex->GetDevice(&spDev);
        if (spDev != m_spD3DDevice)
        {
            fNativeOut = false;
        }
    }


    if (!fNativeIn)
    {
        // Native DX texture in the buffer failed
        // create a texture we can use and copy the data in
        if (m_spInBufferTex == nullptr)
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = m_outputWidth;
            desc.Height = m_outputHeight;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;

            ThrowIfError(m_spD3DDevice->CreateTexture2D(&desc, nullptr, &m_spInBufferTex));
        }

        // scope the texture lock
        {
            TextureLock tlock(m_spD3DContext.Get(), m_spInBufferTex.Get());
            ThrowIfError(tlock.Map(uiInIndex, D3D11_MAP_WRITE_DISCARD, 0));

            // scope the video buffer lock
            {
                LONG lStride = GetDefaultStride(m_spInputType.Get());

                VideoBufferLock lock(pIn, MF2DBuffer_LockFlags_Read, m_outputHeight, lStride);

                ThrowIfError(MFCopyImage((BYTE*)tlock.map.pData, tlock.map.RowPitch, lock.GetData(), lock.GetStride(), abs(lStride), m_outputHeight));
            }
        }
        spInTex = m_spInBufferTex;
    }

    if (!fNativeOut)
    {
        if (m_spOutBufferTex == nullptr)
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = m_outputWidth;
            desc.Height = m_outputHeight;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            ThrowIfError(m_spD3DDevice->CreateTexture2D(&desc, nullptr, &m_spOutBufferTex));
        }
        spOutTex = m_spOutBufferTex;
    }

    // do some processing
    m_frameMixer.ProcessFrame(m_spD3DDevice.Get(), spInTex.Get(), uiInIndex, spOutTex.Get(), uiOutIndex);

    //m_spD3DContext->CopyResource(spOutTex.Get(), spInTex.Get());

    // write back pOut if necessary
    if (!fNativeOut)
    {
        if (m_spOutBufferStage == nullptr)
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = m_outputWidth;
            desc.Height = m_outputHeight;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            ThrowIfError(m_spD3DDevice->CreateTexture2D(&desc, nullptr, &m_spOutBufferStage));
        }

        m_spD3DContext->CopyResource(m_spOutBufferStage.Get(), m_spOutBufferTex.Get());

        {
            TextureLock tlock(m_spD3DContext.Get(), m_spOutBufferStage.Get());
            ThrowIfError(tlock.Map(uiOutIndex, D3D11_MAP_READ, 0));

            // scope the video buffer lock
            {
                LONG lStride = GetDefaultStride(m_spOutputType.Get());

                VideoBufferLock lock(pOut, MF2DBuffer_LockFlags_Write, m_outputHeight, lStride);

                ThrowIfError(MFCopyImage(lock.GetData(), lock.GetStride(), (BYTE*)tlock.map.pData, tlock.map.RowPitch, abs(lStride), m_outputHeight));
            }
        }
    }
}

// Generate output data.
void CAdvancedMediaSource::ProcessOutput(IMFMediaBuffer *pOut)
{
    ComPtr<ID3D11Texture2D> spOutTex;
    UINT uiOutIndex = 0;

    // Attempt to convert directly to DX textures
    try
    {
        spOutTex = BufferToDXType(pOut, &uiOutIndex);
    }
    catch (Exception^)
    {
    }

    bool fNativeOut = spOutTex != nullptr;

    // If the input or output textures' device does not match our device
    // we have to move them to our device
    if (fNativeOut)
    {
        ComPtr<ID3D11Device> spDev;
        spOutTex->GetDevice(&spDev);
        if (spDev != m_spD3DDevice)
        {
            fNativeOut = false;
        }
    }

    if (!fNativeOut)
    {
        if (m_spOutBufferTex == nullptr)
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = m_outputWidth;
            desc.Height = m_outputHeight;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            ThrowIfError(m_spD3DDevice->CreateTexture2D(&desc, nullptr, &m_spOutBufferTex));
        }
        spOutTex = m_spOutBufferTex;
    }

    // do some processing
    m_frameMixer.ProcessFrame(m_spD3DDevice.Get(), spOutTex.Get(), uiOutIndex);

    //m_spD3DContext->CopyResource(spOutTex.Get(), spInTex.Get());

    // write back pOut if necessary
    if (!fNativeOut)
    {
        if (m_spOutBufferStage == nullptr)
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = m_outputWidth;
            desc.Height = m_outputHeight;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            ThrowIfError(m_spD3DDevice->CreateTexture2D(&desc, nullptr, &m_spOutBufferStage));
        }

        m_spD3DContext->CopyResource(m_spOutBufferStage.Get(), m_spOutBufferTex.Get());

        {
            TextureLock tlock(m_spD3DContext.Get(), m_spOutBufferStage.Get());
            ThrowIfError(tlock.Map(uiOutIndex, D3D11_MAP_READ, 0));

            // scope the video buffer lock
            {
                LONG lStride = GetDefaultStride(m_spOutputType.Get());

                VideoBufferLock lock(pOut, MF2DBuffer_LockFlags_Write, m_outputHeight, lStride);

                ThrowIfError(MFCopyImage(lock.GetData(), lock.GetStride(), (BYTE*)tlock.map.pData, tlock.map.RowPitch, abs(lStride), m_outputHeight));
            }
        }
    }
}
