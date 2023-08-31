#pragma once
#include "BaseDefs/DX11.h"

template<
    BOOL DepthEnable = true,
    BOOL DepthWriteEnable = true,
    D3D11_COMPARISON_FUNC DepthFunc = D3D11_COMPARISON_LESS,

    BOOL StencilEnable = false,
    UINT8 StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
    UINT8 StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
    D3D11_STENCIL_OP FrontfaceStencilFailOp = D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP FrontfaceStencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP FrontfaceStencilPassOp = D3D11_STENCIL_OP_KEEP,
    D3D11_COMPARISON_FUNC FrontfaceStencilFunc = D3D11_COMPARISON_ALWAYS,
    D3D11_STENCIL_OP BackfaceStencilFailOp = D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP BackfaceStencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP BackfaceStencilPassOp = D3D11_STENCIL_OP_KEEP,
    D3D11_COMPARISON_FUNC BackfaceStencilFunc = D3D11_COMPARISON_ALWAYS
>
class NXDepthStencilState
{
public:
    static ID3D11DepthStencilState* Create()
    {
        ID3D11DepthStencilState* pDepthStencilState = nullptr;
        CD3D11_DEPTH_STENCIL_DESC desc(
            DepthEnable, 
            DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO,
            DepthFunc, 
            StencilEnable, 
            StencilReadMask, 
            StencilWriteMask, 
            FrontfaceStencilFailOp, 
            FrontfaceStencilDepthFailOp, 
            FrontfaceStencilPassOp, 
            FrontfaceStencilFunc, 
            BackfaceStencilFailOp, 
            BackfaceStencilDepthFailOp, 
            BackfaceStencilPassOp, 
            BackfaceStencilFunc
        );

        NX::ThrowIfFailed(g_pDevice->CreateDepthStencilState(&desc, &pDepthStencilState));
        return pDepthStencilState;
    }
};

template<
    BOOL            AlphaToCoverageEnable       = false,
    BOOL            IndependentBlendEnable      = false,
    BOOL            RT0BlendEnable              = false,
    D3D11_BLEND     RT0SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT0DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT0BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT0SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT0DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT0BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT0RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT1BlendEnable              = false,
    D3D11_BLEND     RT1SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT1DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT1BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT1SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT1DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT1BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT1RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT2BlendEnable              = false,
    D3D11_BLEND     RT2SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT2DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT2BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT2SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT2DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT2BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT2RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT3BlendEnable              = false,
    D3D11_BLEND     RT3SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT3DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT3BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT3SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT3DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT3BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT3RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT4BlendEnable              = false,
    D3D11_BLEND     RT4SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT4DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT4BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT4SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT4DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT4BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT4RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT5BlendEnable              = false,
    D3D11_BLEND     RT5SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT5DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT5BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT5SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT5DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT5BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT5RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT6BlendEnable              = false,
    D3D11_BLEND     RT6SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT6DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT6BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT6SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT6DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT6BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT6RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL,
    BOOL            RT7BlendEnable              = false,
    D3D11_BLEND     RT7SrcBlend                 = D3D11_BLEND_ONE,
    D3D11_BLEND     RT7DestBlend                = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT7BlendOp                  = D3D11_BLEND_OP_ADD,
    D3D11_BLEND     RT7SrcBlendAlpha            = D3D11_BLEND_ONE,
    D3D11_BLEND     RT7DestBlendAlpha           = D3D11_BLEND_ZERO,
    D3D11_BLEND_OP  RT7BlendOpAlpha             = D3D11_BLEND_OP_ADD,
    UINT8           RT7RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL
>
class NXBlendState
{
public:
    static ID3D11BlendState* Create()
    {
        ID3D11BlendState* pBlendState = nullptr;
        D3D11_BLEND_DESC desc;

        desc.AlphaToCoverageEnable                   = AlphaToCoverageEnable;
        desc.IndependentBlendEnable                  = IndependentBlendEnable;

        desc.RenderTarget[0].BlendEnable             = RT0BlendEnable;
        desc.RenderTarget[0].SrcBlend                = RT0SrcBlend;
        desc.RenderTarget[0].DestBlend               = RT0DestBlend;
        desc.RenderTarget[0].BlendOp                 = RT0BlendOp;
        desc.RenderTarget[0].SrcBlendAlpha           = RT0SrcBlendAlpha;
        desc.RenderTarget[0].DestBlendAlpha          = RT0DestBlendAlpha;
        desc.RenderTarget[0].BlendOpAlpha            = RT0BlendOpAlpha;
        desc.RenderTarget[0].RenderTargetWriteMask   = RT0RenderTargetWriteMask;

        desc.RenderTarget[1].BlendEnable             = RT1BlendEnable;
        desc.RenderTarget[1].SrcBlend                = RT1SrcBlend;
        desc.RenderTarget[1].DestBlend               = RT1DestBlend;
        desc.RenderTarget[1].BlendOp                 = RT1BlendOp;
        desc.RenderTarget[1].SrcBlendAlpha           = RT1SrcBlendAlpha;
        desc.RenderTarget[1].DestBlendAlpha          = RT1DestBlendAlpha;
        desc.RenderTarget[1].BlendOpAlpha            = RT1BlendOpAlpha;
        desc.RenderTarget[1].RenderTargetWriteMask   = RT1RenderTargetWriteMask;

        desc.RenderTarget[2].BlendEnable             = RT2BlendEnable;
        desc.RenderTarget[2].SrcBlend                = RT2SrcBlend;
        desc.RenderTarget[2].DestBlend               = RT2DestBlend;
        desc.RenderTarget[2].BlendOp                 = RT2BlendOp;
        desc.RenderTarget[2].SrcBlendAlpha           = RT2SrcBlendAlpha;
        desc.RenderTarget[2].DestBlendAlpha          = RT2DestBlendAlpha;
        desc.RenderTarget[2].BlendOpAlpha            = RT2BlendOpAlpha;
        desc.RenderTarget[2].RenderTargetWriteMask   = RT2RenderTargetWriteMask;

        desc.RenderTarget[3].BlendEnable             = RT3BlendEnable;
        desc.RenderTarget[3].SrcBlend                = RT3SrcBlend;
        desc.RenderTarget[3].DestBlend               = RT3DestBlend;
        desc.RenderTarget[3].BlendOp                 = RT3BlendOp;
        desc.RenderTarget[3].SrcBlendAlpha           = RT3SrcBlendAlpha;
        desc.RenderTarget[3].DestBlendAlpha          = RT3DestBlendAlpha;
        desc.RenderTarget[3].BlendOpAlpha            = RT3BlendOpAlpha;
        desc.RenderTarget[3].RenderTargetWriteMask   = RT3RenderTargetWriteMask;

        desc.RenderTarget[4].BlendEnable             = RT4BlendEnable;
        desc.RenderTarget[4].SrcBlend                = RT4SrcBlend;
        desc.RenderTarget[4].DestBlend               = RT4DestBlend;
        desc.RenderTarget[4].BlendOp                 = RT4BlendOp;
        desc.RenderTarget[4].SrcBlendAlpha           = RT4SrcBlendAlpha;
        desc.RenderTarget[4].DestBlendAlpha          = RT4DestBlendAlpha;
        desc.RenderTarget[4].BlendOpAlpha            = RT4BlendOpAlpha;
        desc.RenderTarget[4].RenderTargetWriteMask   = RT4RenderTargetWriteMask;

        desc.RenderTarget[5].BlendEnable             = RT5BlendEnable;
        desc.RenderTarget[5].SrcBlend                = RT5SrcBlend;
        desc.RenderTarget[5].DestBlend               = RT5DestBlend;
        desc.RenderTarget[5].BlendOp                 = RT5BlendOp;
        desc.RenderTarget[5].SrcBlendAlpha           = RT5SrcBlendAlpha;
        desc.RenderTarget[5].DestBlendAlpha          = RT5DestBlendAlpha;
        desc.RenderTarget[5].BlendOpAlpha            = RT5BlendOpAlpha;
        desc.RenderTarget[5].RenderTargetWriteMask   = RT5RenderTargetWriteMask;

        desc.RenderTarget[6].BlendEnable             = RT6BlendEnable;
        desc.RenderTarget[6].SrcBlend                = RT6SrcBlend;
        desc.RenderTarget[6].DestBlend               = RT6DestBlend;
        desc.RenderTarget[6].BlendOp                 = RT6BlendOp;
        desc.RenderTarget[6].SrcBlendAlpha           = RT6SrcBlendAlpha;
        desc.RenderTarget[6].DestBlendAlpha          = RT6DestBlendAlpha;
        desc.RenderTarget[6].BlendOpAlpha            = RT6BlendOpAlpha;
        desc.RenderTarget[6].RenderTargetWriteMask   = RT6RenderTargetWriteMask;

        desc.RenderTarget[7].BlendEnable             = RT7BlendEnable;
        desc.RenderTarget[7].SrcBlend                = RT7SrcBlend;
        desc.RenderTarget[7].DestBlend               = RT7DestBlend;
        desc.RenderTarget[7].BlendOp                 = RT7BlendOp;
        desc.RenderTarget[7].SrcBlendAlpha           = RT7SrcBlendAlpha;
        desc.RenderTarget[7].DestBlendAlpha          = RT7DestBlendAlpha;
        desc.RenderTarget[7].BlendOpAlpha            = RT7BlendOpAlpha;
        desc.RenderTarget[7].RenderTargetWriteMask   = RT7RenderTargetWriteMask;

        NX::ThrowIfFailed(g_pDevice->CreateBlendState(&desc, &pBlendState));
        return pBlendState;
    }
};

template<
    D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID,
    D3D11_CULL_MODE CullMode = D3D11_CULL_BACK,
    BOOL FrontCounterClockwise = FALSE,
    BOOL DepthClipEnable = TRUE,
    BOOL ScissorEnable = FALSE,
    BOOL MultisampleEnable = FALSE,
    BOOL AntialiasedLineEnable = FALSE
>
class NXRasterizerState
{
public:
    static ID3D11RasterizerState* Create(
        INT     DepthBias               = D3D11_DEFAULT_DEPTH_BIAS, 
        FLOAT   DepthBiasClamp          = D3D11_DEFAULT_DEPTH_BIAS_CLAMP, 
        FLOAT   SlopeScaledDepthBias    = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS)
    {
        ID3D11RasterizerState* pRasterizerState = nullptr;
        CD3D11_RASTERIZER_DESC desc(
            FillMode, 
            CullMode, 
            FrontCounterClockwise, 
            DepthBias, 
            DepthBiasClamp, 
            SlopeScaledDepthBias, 
            DepthClipEnable, 
            ScissorEnable, 
            MultisampleEnable, 
            AntialiasedLineEnable
        );

        NX::ThrowIfFailed(g_pDevice->CreateRasterizerState(&desc, &pRasterizerState));
        return pRasterizerState;
    }
};
