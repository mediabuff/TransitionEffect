#pragma once

#include <vector>
#include "CritSec.h"
#include "CSource.h"
#include "FrameMixer.h"
#include "TransitionManager.h"

//-----------------------------------------------------------------------------
// Constant buffer data
//-----------------------------------------------------------------------------
struct ModelViewProjectionConstantBuffer
{
    DirectX::XMFLOAT4X4 model;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

//-----------------------------------------------------------------------------
// Per-vertex data
//-----------------------------------------------------------------------------
struct VertexPositionColor
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 color;
};

namespace AdvancedMediaSource
{
    public ref class CAdvancedMediaSource sealed
    {
    public:

        CAdvancedMediaSource();
        virtual ~CAdvancedMediaSource();

        void ResetTimeline();
        void AddVideo(Platform::String^ url);
        void AddTransitionEffect(TransitionEffectType type, float length_in_second);

        void Initialize(MediaStreamSource ^ mss, VideoStreamDescriptor ^ videoDesc, AudioStreamDescriptor ^ audioDesc);

        void GenerateVideoSample(Windows::Media::Core::MediaStreamSourceSampleRequest ^ request);
        void GenerateAudioSample(Windows::Media::Core::MediaStreamSourceSampleRequest ^ request);

    private:
        HRESULT GenerateFrame(UINT32 ui32Width, UINT32 ui32Height, int iFrameRotation, IMFMediaBuffer * ppBuffer);
        HRESULT GenerateFrame2(UINT32 ui32Width, UINT32 ui32Height, ULONGLONG curr_time_stamp, IMFMediaBuffer * ppBuffer);
        HRESULT ConvertPropertiesToMediaType(_In_ Windows::Media::MediaProperties::IMediaEncodingProperties ^mep, _Outptr_ IMFMediaType **ppMT);
        static HRESULT AddAttribute(_In_ GUID guidKey, _In_ Windows::Foundation::IPropertyValue ^value, _In_ IMFAttributes *pAttr);
        bool InitialSourceSteams();
        bool InitializeSource();
        void ProcessOutput(IMFMediaBuffer *pIn, IMFMediaBuffer *pOut);
        void ProcessOutput(IMFMediaBuffer *pOut);

        ComPtr<IMFVideoSampleAllocator> m_spSampleAllocator;
        ComPtr<IMFDXGIDeviceManager> m_spDeviceManager;
        ComPtr<ID3D11Device> m_spD3DDevice;
        ComPtr<ID3D11DeviceContext> m_spD3DContext;
        HANDLE m_hDevice;
        ULONGLONG m_ulVideoTimestamp;
        ULONGLONG m_ulAudioTimestamp;
        int m_iVideoFrameNumber;
        UINT32 m_outputWidth;
        UINT32 m_outputHeight;


        CritSec m_critSec;

        bool m_SourceInitialized;
        bool m_EndOfStream;
        std::vector<Platform::String^> m_VideoList;
        std::vector<TransitionEffect> m_TransitionEffectList;
        std::vector<CSourcePtr> m_Sources;
        int m_CurrentVideo;
        int m_CurrentAudio; // MediaStreamSource requests (and buffers) audio and video samples separately. 

        ComPtr<ID3D11Texture2D> m_spInBufferTex;
        ComPtr<ID3D11Texture2D> m_spOutBufferTex;
        ComPtr<ID3D11Texture2D> m_spOutBufferStage;

        // Streaming
        ComPtr<IMFMediaType> m_spInputType;         // Input media type.
        ComPtr<IMFMediaType> m_spOutputType;        // Output media type.
        ComPtr<IMFMediaType> m_spAudioType;        // Output media type.
        
        // Transform
        FrameMixer m_frameMixer;

        TransitionManager m_transition_manager;
    };
}
