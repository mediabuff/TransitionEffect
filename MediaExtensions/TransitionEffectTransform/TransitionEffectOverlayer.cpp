#include "pch.h"
#include "TransitionEffectOverlayer.h"
#include "d2d1effects.h"

using namespace D2D1;

TransitionEffectOverlayer::TransitionEffectOverlayer()
    : m_is_initialized(FALSE)
    , m_uiWidth(0)
    , m_uiHeight(0)
{
    _DPRINTF((L"TransitionEffectOverlayer::TransitionEffectOverlayer"));
}

TransitionEffectOverlayer::~TransitionEffectOverlayer()
{
    _DPRINTF((L"TransitionEffectOverlayer::~TransitionEffectOverlayer"));
}

void TransitionEffectOverlayer::Invalidate()
{

}

void TransitionEffectOverlayer::Initialize(ID2D1Factory2 *pd2dFactory, ID2D1Device1 *pDevice, ID2D1DeviceContext1 *pContext,
                                   IDWriteFactory2 *pDWrite, IWICImagingFactory2 *pwicFactory, UINT uiWidth, UINT uiHeight)
{
    if (m_is_initialized)
        return;

    m_d2dFactory = pd2dFactory;
    m_d2dDevice = pDevice;
    m_d2dDeviceContext = pContext;
    m_dwriteFactory = pDWrite;
    m_wicFactory = pwicFactory;
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    m_is_initialized = TRUE;
}

void TransitionEffectOverlayer::ProcessFrame(ID3D11Texture2D *pTexture2D, UINT uiInIndex, TransitionEffectParameter^ parameter, ULONGLONG current_time)
{
    m_parameter = parameter;

    ComPtr<ID3D11Texture2D> textureOutput = pTexture2D;

    // Use D2D to attach to and draw some text to the cube texture.  Note that the cube
    // texture is DPI-independent, so when drawing content to it using D2D, a fixed DPI
    // value should be used.

    const float dxgiDpi = 96.0f;
    m_d2dDeviceContext->SetDpi(dxgiDpi, dxgiDpi);

    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dxgiDpi,
        dxgiDpi
        );

    ComPtr<IDXGISurface> cubeTextureSurface;
    ThrowIfError(
        textureOutput.As(&cubeTextureSurface)
        );

    ThrowIfError(
        m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
        cubeTextureSurface.Get(),
        &bitmapProperties,
        &m_d2dTargetBitmap
        )
        );

    D2D1_SIZE_U size;
    size.width = (UINT)m_d2dTargetBitmap->GetSize().width;
    size.height = (UINT)m_d2dTargetBitmap->GetSize().height;

    D2D1_BITMAP_PROPERTIES properties;
    properties.pixelFormat = m_d2dTargetBitmap->GetPixelFormat();
    m_d2dTargetBitmap->GetDpi(&properties.dpiX, &properties.dpiY);

    m_d2dDeviceContext->CreateBitmap(size, properties, &m_d2dFadeOutBitmap);

    m_d2dDeviceContext->SetTarget(m_d2dTargetBitmap.Get());

    DrawTransitionEffect(current_time);
}

void TransitionEffectOverlayer::DrawTransitionEffect(ULONGLONG current_time)
{
    TransitionEffectType effect_in_type = m_parameter->GetStartEffect().m_effect;
    ULONGLONG effect_in_duration = m_parameter->GetStartEffect().m_length;
    ULONGLONG effect_in_start_time = 0;
    ULONGLONG effect_in_end_time = effect_in_start_time + effect_in_duration;

    TransitionEffectType effect_out_type = m_parameter->GetEndEffect().m_effect;
    ULONGLONG effect_out_duration = m_parameter->GetEndEffect().m_length;
    ULONGLONG effect_out_end_time = m_parameter->GetVideoDuration();
    ULONGLONG effect_out_start_time = effect_out_end_time - effect_out_duration;

    // effect in
    if (current_time >= effect_in_start_time && current_time < effect_in_end_time)
    {
        FLOAT effect_weight = (FLOAT)(current_time - effect_in_start_time) / (FLOAT)effect_in_duration;

        FadeEffect(effect_in_type, effect_weight);
        MotionEffect(effect_in_type, effect_weight);
    }
    else if (current_time >= effect_out_start_time && current_time < effect_out_end_time)
    {
        FLOAT effect_weight = (FLOAT)(effect_out_end_time - current_time) / (FLOAT)effect_out_duration;

        FadeEffect(effect_out_type, effect_weight);
        MotionEffect(effect_out_type, effect_weight);
    }
}

void TransitionEffectOverlayer::FadeEffect(TransitionEffectType effect_type, FLOAT fadeout_target_weight)
{
    if (effect_type != TransitionEffectType::TRANSITION_FADE_TO_BLACK &&
        effect_type != TransitionEffectType::TRANSITION_FADE_TO_WHITE)
        return;

    ComPtr<ID2D1Effect> targetColorEffect;
    ComPtr<ID2D1Effect> blendColorEffect;
    ThrowIfError(
        m_d2dDeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &targetColorEffect)
        );
    ThrowIfError(
        m_d2dDeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &blendColorEffect)
        );

    FLOAT blend_color = 0.f;
    if (TransitionEffectType::TRANSITION_FADE_TO_BLACK == effect_type)
        blend_color = 0.f;
    else if (TransitionEffectType::TRANSITION_FADE_TO_WHITE == effect_type)
        blend_color = 1.f;

    D2D1_MATRIX_5X4_F colorMatrix = { 0 };
    colorMatrix.m[4][0] = blend_color;
    colorMatrix.m[4][1] = blend_color;
    colorMatrix.m[4][2] = blend_color;
    colorMatrix.m[4][3] = 1.f;
    blendColorEffect->SetInput(0, m_d2dFadeOutBitmap.Get());
    blendColorEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, colorMatrix);
    blendColorEffect->SetValue(D2D1_COLORMATRIX_PROP_ALPHA_MODE, D2D1_COLORMATRIX_ALPHA_MODE_STRAIGHT);

    D2D1_MATRIX_5X4_F alphaMatrix = { 0 };
    alphaMatrix.m[0][0] = 1.f - fadeout_target_weight;
    alphaMatrix.m[1][1] = 1.f - fadeout_target_weight;
    alphaMatrix.m[2][2] = 1.f - fadeout_target_weight;
    alphaMatrix.m[3][3] = 1.f - fadeout_target_weight;
    targetColorEffect->SetInputEffect(0, blendColorEffect.Get());
    targetColorEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, alphaMatrix);
    targetColorEffect->SetValue(D2D1_COLORMATRIX_PROP_ALPHA_MODE, D2D1_COLORMATRIX_ALPHA_MODE_STRAIGHT);

    m_d2dDeviceContext->SetTarget(m_d2dTargetBitmap.Get());
    m_d2dDeviceContext->BeginDraw();
    m_d2dDeviceContext->DrawImage(targetColorEffect.Get());
    m_d2dDeviceContext->EndDraw();
}

void TransitionEffectOverlayer::MotionEffect(TransitionEffectType effect_type, FLOAT fadeout_target_weight)
{
    if (effect_type != TransitionEffectType::TRANSITION_BOTTOM_TO_TOP &&
        effect_type != TransitionEffectType::TRANSITION_RIGHT_TO_LEFT &&
        effect_type != TransitionEffectType::TRANSITION_TOP_TO_BOTTOM &&
        effect_type != TransitionEffectType::TRANSITION_ROOM_IN)
        return;

    // copy original
    D2D1_SIZE_U size;
    size.width = (UINT)m_d2dTargetBitmap->GetSize().width;
    size.height = (UINT)m_d2dTargetBitmap->GetSize().height;

    D2D1_POINT_2U point = { 0, 0 };
    D2D1_RECT_U rect = { 0, 0, size.width, size.height };

    m_d2dFadeOutBitmap->CopyFromBitmap(&point, m_d2dTargetBitmap.Get(), &rect);

    // make background black
    ComPtr<ID2D1Effect> blendColorEffect;
    ThrowIfError(
        m_d2dDeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &blendColorEffect)
        );

    D2D1_MATRIX_5X4_F colorMatrix = { 0 };
    colorMatrix.m[4][0] = 0.f;
    colorMatrix.m[4][1] = 0.f;
    colorMatrix.m[4][2] = 0.f;
    colorMatrix.m[4][3] = 1.f;
    blendColorEffect->SetInput(0, m_d2dFadeOutBitmap.Get());
    blendColorEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, colorMatrix);
    blendColorEffect->SetValue(D2D1_COLORMATRIX_PROP_ALPHA_MODE, D2D1_COLORMATRIX_ALPHA_MODE_STRAIGHT);

    m_d2dDeviceContext->SetTarget(m_d2dTargetBitmap.Get());
    m_d2dDeviceContext->BeginDraw();
    m_d2dDeviceContext->DrawImage(blendColorEffect.Get());
    m_d2dDeviceContext->EndDraw();

    // transform
    ComPtr<ID2D1Effect> affineTransformEffect;
    ThrowIfError(
        m_d2dDeviceContext->CreateEffect(CLSID_D2D12DAffineTransform, &affineTransformEffect)
        );

    affineTransformEffect->SetInput(0, m_d2dFadeOutBitmap.Get());

    D2D1_MATRIX_3X2_F matrix = { 0 };

    FLOAT shift_weight = 1.f - fadeout_target_weight;
    FLOAT room_weight = max(fadeout_target_weight, 0.000001);

    if (TransitionEffectType::TRANSITION_BOTTOM_TO_TOP == effect_type)
        matrix = D2D1::Matrix3x2F(1.0f, 0.0f, 0.0f, 1.0f, 0.0, -shift_weight * 2 * m_uiHeight);
    else if (TransitionEffectType::TRANSITION_RIGHT_TO_LEFT == effect_type)
        matrix = D2D1::Matrix3x2F(1.0f, 0.0f, 0.0f, 1.0f, shift_weight * 2 * m_uiWidth, 0.0f);
    else if (TransitionEffectType::TRANSITION_TOP_TO_BOTTOM == effect_type)
        matrix = D2D1::Matrix3x2F(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, shift_weight * 2 * m_uiHeight);
    else if (TransitionEffectType::TRANSITION_ROOM_IN == effect_type)
        matrix = D2D1::Matrix3x2F(room_weight, 0.0f, 0.0f, room_weight, m_uiWidth / 2 * (1 - room_weight), m_uiHeight / 2 * (1 - room_weight));

    affineTransformEffect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, matrix);

    m_d2dDeviceContext->SetTarget(m_d2dTargetBitmap.Get());
    m_d2dDeviceContext->BeginDraw();
    m_d2dDeviceContext->DrawImage(affineTransformEffect.Get());
    m_d2dDeviceContext->EndDraw();
}