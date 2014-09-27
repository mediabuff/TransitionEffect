// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once
#include "VideoTypes.h"

//-----------------------------------------------------------------------------
// Constant buffer data
//-----------------------------------------------------------------------------
struct VSTransformBuffer
{
    DirectX::XMFLOAT4X4 transform;
};

struct PSFadingBuffer
{
    float fading;
    float memory_pading[7];
};

namespace AdvancedMediaSource
{
    class FrameMixer
    {
    public:
        FrameMixer();
        virtual ~FrameMixer();

        void Invalidate();
        void Initialize(ID3D11Device *pDevice, UINT uiWidth, UINT uiHeight);
        void ProcessFrame(ID3D11Device *pDevice, ID3D11Texture2D *pInput, UINT uiInIndex, ID3D11Texture2D *pOutput, UINT uiOutIndex);
        void ProcessFrame(ID3D11Device *pDevice, ID3D11Texture2D *pOutput, UINT uiOutIndex);

        void SetInputSize(ID3D11DeviceContext *pContext, UINT uiWidth, UINT uiHeight);

        void SetTransitionparameter(TransitionParameter parameter) { m_transition_parameter = parameter; };

    private:
        UINT m_uiInputWidth;
        UINT m_uiInputHeight;
        UINT m_uiInputNewWidth;
        UINT m_uiInputNewHeight;
        UINT m_uiWidth;
        UINT m_uiHeight;
        ComPtr<ID3D11Buffer> m_spScreenQuadVB;
        ComPtr<ID3D11SamplerState> m_spSampleStateLinear;
        ComPtr<ID3D11InputLayout> m_spQuadLayout;
        ComPtr<ID3D11VertexShader> m_spVertexShader;
        ComPtr<ID3D11PixelShader> m_spPixelShader;
        ComPtr<ID3D11BlendState> m_spBlendStateNoBlend;

        // DirectX Objects
        VSTransformBuffer m_vsTransformBufferData;
        PSFadingBuffer m_psFadingBufferData;
        ComPtr<ID3D11Buffer> m_vsTransformBuffer;
        ComPtr<ID3D11Buffer> m_psFadingBuffer;

        TransitionParameter m_transition_parameter;

        Platform::String^              m_sampleName;

        void ConfigureBlendingStage(ID3D11Device *pd3dDevice);
        void InitialDirectXObject(ID3D11Device *pd3dDevice);
    };
}
