#include "pch.h"
#include "TransitionEffectModule.h"

using namespace D2D1;

CTransitionEffectModule::CTransitionEffectModule()
    : m_uiWidth(0)
    , m_uiHeight(0)
{
    CreateDeviceIndependentResources();
}

CTransitionEffectModule::~CTransitionEffectModule()
{

}

void CTransitionEffectModule::Invalidate()
{
    m_overlayer.Invalidate();
}

void CTransitionEffectModule::SetTransitionEffectParameter(TransitionEffectParameter^ parameter)
{
    m_parameter = parameter;
}

void CTransitionEffectModule::Initialize(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, UINT uiWidth, UINT uiHeight)
{
    m_d3dDevice = pDevice;
    m_d3dDeviceContext = pContext;
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    ComPtr<IDXGIDevice> dxgiDevice;
    // Get the underlying DXGI device of the Direct3D device.
    ThrowIfError(
        m_d3dDevice.As(&dxgiDevice)
        );


    // Create the Direct2D device object and a corresponding context.
    ThrowIfError(
        m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
        );

    ThrowIfError(
        m_d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &m_d2dDeviceContext
        )
        );
}

void CTransitionEffectModule::ProcessFrame(ID3D11Texture2D *pInput,
                                 UINT uiInIndex,
                                 ID3D11Texture2D *pOutput,
                                 UINT uiOutIndex,
                                 LONGLONG curr_time)
{
    ComPtr<ID3D11Texture2D> textureInput = pInput;
    ComPtr<ID3D11Texture2D> textureOutput = pOutput;

    m_d3dDeviceContext->CopyResource(textureOutput.Get(), textureInput.Get());

    m_overlayer.Initialize(m_d2dFactory.Get(), m_d2dDevice.Get(), m_d2dDeviceContext.Get(), m_dwriteFactory.Get(),
                            m_wicFactory.Get(), m_uiWidth, m_uiHeight);

    if (nullptr == m_parameter)
        return;

    m_overlayer.ProcessFrame(textureOutput.Get(), uiOutIndex, m_parameter, (ULONGLONG)curr_time);
}

void CTransitionEffectModule::CreateDeviceIndependentResources()
{
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
    // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    ThrowIfError(
        D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory2),
        &options,
        &m_d2dFactory
        )
        );

    ThrowIfError(
        DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory2),
        &m_dwriteFactory
        )
        );

    ThrowIfError(
        CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory)
        )
        );
}

