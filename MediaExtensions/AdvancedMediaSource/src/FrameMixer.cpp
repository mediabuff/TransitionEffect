// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"
#include "FrameMixer.h"

using namespace DirectX;

struct ScreenVertex
{
    FLOAT pos[4];
    FLOAT tex[2];
};

void FrameMixer::Invalidate()
{
    m_spScreenQuadVB.Reset();
    m_spSampleStateLinear.Reset();
    m_spQuadLayout.Reset();
    m_spVertexShader.Reset();
    m_spPixelShader.Reset();
}

void FrameMixer::Initialize(ID3D11Device *pDevice, UINT uiWidth, UINT uiHeight)
{
    try
    {
        m_uiWidth = uiWidth;
        m_uiHeight = uiHeight;
        m_uiInputWidth = uiWidth;
        m_uiInputHeight = uiHeight;
        m_uiInputNewWidth = uiWidth;
        m_uiInputNewHeight = uiHeight;

        if (m_spScreenQuadVB == nullptr)
        {
            // Vertex Buffer Layout
            D3D11_BUFFER_DESC vbdesc;

            // 2 vertices (Triangles), covering the whole Viewport, with the input video mapped as a texture
            const ScreenVertex svDefault[4] = {
                //   x      y     z     w         u     v
                { { -1.0f,  1.0f, 0.5f, 1.0f }, { 0.0f, 0.0f } }, // 0
                { {  1.0f,  1.0f, 0.5f, 1.0f }, { 1.0f, 0.0f } }, // 1
                { { -1.0f, -1.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } }, // 2
                { {  1.0f, -1.0f, 0.5f, 1.0f }, { 1.0f, 1.0f } }  // 3
            };

            ZeroMemory(&vbdesc, sizeof(vbdesc));

            vbdesc.Usage = D3D11_USAGE_DYNAMIC;
            vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vbdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            vbdesc.MiscFlags = 0;
            vbdesc.ByteWidth = sizeof( svDefault );

            D3D11_SUBRESOURCE_DATA InitData;
            InitData.pSysMem = svDefault;
            InitData.SysMemPitch = 0;
            InitData.SysMemSlicePitch = 0;
            ThrowIfError(pDevice->CreateBuffer(&vbdesc, &InitData, &m_spScreenQuadVB));
        }

        // Pixel Shader
        {
            static
    #       include "PixelShader.hps"
            if (m_spPixelShader == nullptr)
            {
                ThrowIfError(pDevice->CreatePixelShader(g_psshader, sizeof(g_psshader), nullptr, &m_spPixelShader));
            }
        }

        // Vertex Shader
        {
            static
    #       include "VertexShader.hvs"
            if (m_spVertexShader == nullptr)
            {
                const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
                {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                ThrowIfError(pDevice->CreateVertexShader(g_vsshader, sizeof(g_vsshader), nullptr, &m_spVertexShader));

                ThrowIfError(pDevice->CreateInputLayout(quadlayout, 2, g_vsshader, sizeof(g_vsshader), &m_spQuadLayout));
            }
        }

        if (m_spSampleStateLinear == nullptr)
        {
            // Sampler
            D3D11_SAMPLER_DESC SamplerDesc;
            ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));

            SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            ThrowIfError(pDevice->CreateSamplerState(&SamplerDesc, &m_spSampleStateLinear));
        }

        if (m_spBlendStateNoBlend == nullptr)
        {
            ConfigureBlendingStage(pDevice);
        }

        InitialDirectXObject(pDevice);
    }
    catch(Exception^)
    {
        Invalidate();
        throw;
    }
}

void FrameMixer::SetInputSize(ID3D11DeviceContext *pContext, UINT uiWidth, UINT uiHeight)
{
    if (m_uiInputWidth != uiWidth || m_uiInputHeight != uiHeight)
    {
        float new_width = 0.f, new_height = 0.f;

        float input_aspect_ratio = (float)uiWidth / (float)uiHeight;
        float output_aspect_ratio = (float)m_uiWidth / (float)m_uiHeight;

        if (input_aspect_ratio >= output_aspect_ratio)
        {
            new_width = (float)m_uiWidth;
            new_height = new_width * uiHeight / uiWidth;
        }
        else
        {
            new_height = (float)m_uiHeight;
            new_width = new_height * uiWidth / uiHeight;
        }

        float dx = ((float)m_uiWidth - new_width) / m_uiWidth;
        float dy = ((float)m_uiHeight - new_height) / m_uiHeight;


        // 2 vertices (Triangles), covering the whole Viewport, with the input video mapped as a texture
        const ScreenVertex svResize[4] = {
            //   x           y          z     w         u     v
            { { -1.0f + dx,  1.0f - dy, 0.5f, 1.0f }, { 0.0f, 0.0f } }, // 0
            { {  1.0f - dx,  1.0f - dy, 0.5f, 1.0f }, { 1.0f, 0.0f } }, // 1
            { { -1.0f + dx, -1.0f + dy, 0.5f, 1.0f }, { 0.0f, 1.0f } }, // 2
            { {  1.0f - dx, -1.0f + dy, 0.5f, 1.0f }, { 1.0f, 1.0f } }  // 3
        };

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        pContext->Map(m_spScreenQuadVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &svResize, sizeof(svResize));
        pContext->Unmap(m_spScreenQuadVB.Get(), 0);

        m_uiInputWidth = uiWidth;
        m_uiInputHeight = uiHeight;
        m_uiInputNewWidth = (int)new_width;
        m_uiInputNewHeight = (int)new_height;

        _DPRINTF((L"m_uiWidth %d, m_uiHeight %d, m_uiInputNewWidth %d, m_uiInputNewHeight %d",
                  m_uiWidth, m_uiHeight, m_uiInputNewWidth, m_uiInputNewHeight));
    }
}

HRESULT CreateShaderInputView(ID3D11Texture2D *pTexture, UINT uiIndex, ID3D11Device *pDevice, ID3D11ShaderResourceView **ppSRV)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;

    ZeroMemory( &viewDesc, sizeof( viewDesc ) );

    pTexture->GetDesc( &textureDesc );

    viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    if ( textureDesc.ArraySize > 1 )
    {
        // Texture array, use the subresource index
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.FirstArraySlice = uiIndex;
        viewDesc.Texture2DArray.ArraySize = 1;
        viewDesc.Texture2DArray.MipLevels = 1;
        viewDesc.Texture2DArray.MostDetailedMip = 0;
    }
    else
    {
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = 1;
    }

    return pDevice->CreateShaderResourceView(pTexture,
                                             &viewDesc,
                                             ppSRV);
}

HRESULT CreateShaderOutputView(ID3D11Texture2D *pTexture, UINT uiIndex, ID3D11Device *pDevice, ID3D11RenderTargetView **ppRTV)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    D3D11_RENDER_TARGET_VIEW_DESC viewDesc;

    ZeroMemory( &viewDesc, sizeof( viewDesc ) );

    pTexture->GetDesc( &textureDesc );

    viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    if ( textureDesc.ArraySize > 1 )
    {
        // Texture array, use the subresource index
        viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.FirstArraySlice = uiIndex;
        viewDesc.Texture2DArray.ArraySize = 1;
        viewDesc.Texture2DArray.MipSlice = 0;
    }
    else
    {
        viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
    }

    return pDevice->CreateRenderTargetView(pTexture,
                                           &viewDesc,
                                           ppRTV );

}

void FrameMixer::ProcessFrame(ID3D11Device *pDevice, ID3D11Texture2D *pInput, UINT uiInIndex, ID3D11Texture2D *pOutput, UINT uiOutIndex)
{
    ComPtr<ID3D11DeviceContext> spd3dImmediateContext;

    // Render targets
    ComPtr<ID3D11RenderTargetView> rgpOrigRTV;
    ComPtr<ID3D11DepthStencilView> pOrigDSV;
    ComPtr<ID3D11RenderTargetView> spRTV;

    // Shader resources
    ComPtr<ID3D11ShaderResourceView> rgpOrigSRV;
    ComPtr<ID3D11ShaderResourceView> spSRV;

    // Vertex buffers
    ID3D11Buffer *pBuffers[1] = { m_spScreenQuadVB.Get() };
    UINT vbStrides = sizeof( ScreenVertex );
    UINT vbOffsets = 0;

    // Samplers
    ID3D11SamplerState *pSamplers[1] = { m_spSampleStateLinear.Get() };

    // View port
    D3D11_VIEWPORT vpOld[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
    UINT nViewPorts = 1;
    D3D11_VIEWPORT vp;

    // Transition parameter;
    TransitionParameter parameter = m_transition_parameter;

    // Get the context
    pDevice->GetImmediateContext(&spd3dImmediateContext);
   
    // Get the resource views
    ThrowIfError(CreateShaderInputView(pInput, uiInIndex, pDevice, &spSRV));

    ThrowIfError(CreateShaderOutputView(pOutput, uiOutIndex, pDevice, &spRTV));

    // Setup the Draw call
    spd3dImmediateContext->IASetInputLayout(m_spQuadLayout.Get());

    spd3dImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &vbStrides, &vbOffsets);

    spd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    spd3dImmediateContext->PSSetSamplers(0, 1, pSamplers);

    spd3dImmediateContext->OMGetRenderTargets(1, &rgpOrigRTV, &pOrigDSV);

    const FLOAT background_color[4] = { parameter.m_backgound_color[0],
                                        parameter.m_backgound_color[1],
                                        parameter.m_backgound_color[2],
                                        parameter.m_backgound_color[3] };

    if (parameter.m_draw_background)
    {
        spd3dImmediateContext->ClearRenderTargetView(spRTV.Get(), background_color);
    }

    spd3dImmediateContext->OMSetRenderTargets(1, spRTV.GetAddressOf(), nullptr);

    // apply transform and fading
    XMStoreFloat4x4(
        &m_vsTransformBufferData.transform,
        XMMatrixTranspose(XMMatrixTranslation(parameter.m_offset_x, parameter.m_offset_y, parameter.m_offset_z) *
        XMMatrixScaling(parameter.m_scaling_x, parameter.m_scaling_y, 1.0))
        );

    m_psFadingBufferData.fading = parameter.m_fading_ratio;

    spd3dImmediateContext->UpdateSubresource(m_vsTransformBuffer.Get(), 0, nullptr, &m_vsTransformBufferData, 0, 0);
    spd3dImmediateContext->UpdateSubresource(m_psFadingBuffer.Get(), 0, nullptr, &m_psFadingBufferData, 0, 0);
    //

    spd3dImmediateContext->PSGetShaderResources(0, 1, &rgpOrigSRV);

    spd3dImmediateContext->PSSetShaderResources(0, 1, spSRV.GetAddressOf());

    spd3dImmediateContext->RSGetViewports(&nViewPorts, vpOld);

    // Setup the viewport to match the backbuffer
    vp.Width = (float)m_uiWidth;
    vp.Height = (float)m_uiHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;

    spd3dImmediateContext->RSSetViewports(1, &vp);

    spd3dImmediateContext->VSSetShader(m_spVertexShader.Get(), nullptr, 0);

    // Set the vertex shader constant buffer data.
    spd3dImmediateContext->VSSetConstantBuffers(0, 1, m_vsTransformBuffer.GetAddressOf());

    spd3dImmediateContext->PSSetShader(m_spPixelShader.Get(), nullptr, 0);

    // Set the pixel shader constant buffer data.
    spd3dImmediateContext->PSSetConstantBuffers(0, 1, m_psFadingBuffer.GetAddressOf());

    UINT sampleMask = 0xffffffff;
    spd3dImmediateContext->OMSetBlendState(m_spBlendStateNoBlend.Get(), background_color, sampleMask);

    spd3dImmediateContext->Draw(4, 0);

    // Restore the Old
    spd3dImmediateContext->RSSetViewports(nViewPorts, vpOld);
    spd3dImmediateContext->PSSetShaderResources(0, 1, rgpOrigSRV.GetAddressOf());
    spd3dImmediateContext->OMSetRenderTargets(1, rgpOrigRTV.GetAddressOf(), pOrigDSV.Get()); 
}

void FrameMixer::ProcessFrame(ID3D11Device *pDevice, ID3D11Texture2D *pOutput, UINT uiOutIndex)
{
    ComPtr<ID3D11DeviceContext> spd3dImmediateContext;

    // Render targets
    ComPtr<ID3D11RenderTargetView> rgpOrigRTV;
    ComPtr<ID3D11DepthStencilView> pOrigDSV;
    ComPtr<ID3D11RenderTargetView> spRTV;

    // Shader resources
    ComPtr<ID3D11ShaderResourceView> rgpOrigSRV;
    ComPtr<ID3D11ShaderResourceView> spSRV;

    // Vertex buffers
    ID3D11Buffer *pBuffers[1] = { m_spScreenQuadVB.Get() };
    UINT vbStrides = sizeof(ScreenVertex);
    UINT vbOffsets = 0;

    // Samplers
    ID3D11SamplerState *pSamplers[1] = { m_spSampleStateLinear.Get() };

    // View port
    UINT nViewPorts = 1;

    // Transition parameter;
    TransitionParameter parameter = m_transition_parameter;

    // Get the context
    pDevice->GetImmediateContext(&spd3dImmediateContext);

    // Get the resource views
    ThrowIfError(CreateShaderOutputView(pOutput, uiOutIndex, pDevice, &spRTV));

    // Setup the Draw call
    const FLOAT background_color[4] = { parameter.m_backgound_color[0],
                                        parameter.m_backgound_color[1],
                                        parameter.m_backgound_color[2],
                                        parameter.m_backgound_color[3] };

    if (parameter.m_draw_background)
    {
        spd3dImmediateContext->ClearRenderTargetView(spRTV.Get(), background_color);
    }
}

void FrameMixer::ConfigureBlendingStage(ID3D11Device *pd3dDevice)
{
    D3D11_BLEND_DESC BlendState;
    ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
    BlendState.RenderTarget[0].BlendEnable = TRUE;
    BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    pd3dDevice->CreateBlendState(&BlendState, &m_spBlendStateNoBlend);
}

void FrameMixer::InitialDirectXObject(ID3D11Device *pd3dDevice)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        CD3D11_BUFFER_DESC constantBufferDesc(
            sizeof(VSTransformBuffer),
            D3D11_BIND_CONSTANT_BUFFER
            );

        hr = pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &m_vsTransformBuffer);
    }

    if (SUCCEEDED(hr))
    {
        CD3D11_BUFFER_DESC constantBufferDesc(
            sizeof(PSFadingBuffer),
            D3D11_BIND_CONSTANT_BUFFER
            );

        hr = pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &m_psFadingBuffer);
    }
}