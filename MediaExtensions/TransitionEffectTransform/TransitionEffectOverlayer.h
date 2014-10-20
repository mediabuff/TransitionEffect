#pragma once

#include "TransitionEffectDef.h"

using namespace TransitionEffectTransform;

ref class TransitionEffectOverlayer sealed
{
public:
    TransitionEffectOverlayer();
    virtual ~TransitionEffectOverlayer();

internal:
    void Invalidate();

    void Initialize(ID2D1Factory2 *pd2dFactory, ID2D1Device1 *pDevice, ID2D1DeviceContext1 *pContext,
                    IDWriteFactory2 *pDWrite, IWICImagingFactory2 *pwicFactory, UINT uiWidth, UINT uiHeight);

    void ProcessFrame(ID3D11Texture2D *pTexture2D, UINT uiInIndex, TransitionEffectParameter^ parameter, ULONGLONG current_time);

private:
    BOOL m_is_initialized;
    UINT32 m_uiWidth;
    UINT32 m_uiHeight;

    TransitionEffectParameter^ m_parameter;

    // Direct2D Rendering Objects. Required for 2D.
    ComPtr<ID2D1Factory2>           m_d2dFactory;
    ComPtr<ID2D1Device1>            m_d2dDevice;
    ComPtr<ID2D1DeviceContext1>     m_d2dDeviceContext;
    ComPtr<ID2D1Bitmap1>            m_d2dTargetBitmap;
    ComPtr<ID2D1Bitmap>             m_d2dFadeOutBitmap;

    // DirectWrite & Windows Imaging Component Objects.
    ComPtr<IDWriteFactory2>         m_dwriteFactory;
    ComPtr<IWICImagingFactory2>     m_wicFactory;

    void InitializeTextFormat();
    void DrawTransitionEffect(ULONGLONG current_time);

    void PrepareFadeEffect();
    void FadeEffect(TransitionEffectType effect_type, FLOAT fadeout_target_weight);
    void MotionEffect(TransitionEffectType effect_type, FLOAT fadeout_target_weight);
};