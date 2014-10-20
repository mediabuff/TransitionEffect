#pragma once

#include "TransitionEffectOverlayer.h"
#include "TransitionEffectDef.h"

using namespace TransitionEffectTransform;

ref class CTransitionEffectModule sealed
{
public:
    CTransitionEffectModule();
    virtual ~CTransitionEffectModule();

internal:
    void Invalidate();
    void Initialize(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, UINT uiWidth, UINT uiHeight);
    void SetTransitionEffectParameter(TransitionEffectParameter^ parameter);
    void ProcessFrame(ID3D11Texture2D *pInput, UINT uiInIndex, ID3D11Texture2D *pOutput, UINT uiOutIndex, LONGLONG curr_time);

private:
    UINT m_uiWidth;
    UINT m_uiHeight;

    TransitionEffectParameter^ m_parameter;

    // Direct3D resources
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<ID3D11DeviceContext> m_d3dDeviceContext;

    // Direct2D Rendering Objects. Required for 2D.
    ComPtr<ID2D1Factory2>           m_d2dFactory;
    ComPtr<ID2D1Device1>            m_d2dDevice;
    ComPtr<ID2D1DeviceContext1>     m_d2dDeviceContext;

    // DirectWrite & Windows Imaging Component Objects.
    ComPtr<IDWriteFactory2>         m_dwriteFactory;
    ComPtr<IWICImagingFactory2>     m_wicFactory;

    void CreateDeviceIndependentResources();

    TransitionEffectOverlayer m_overlayer;
};
