#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include <d3d12.h>

template<
    BOOL DepthEnable = TRUE,
    BOOL DepthWriteEnable = TRUE,
    D3D12_COMPARISON_FUNC DepthFunc = D3D12_COMPARISON_FUNC_LESS,

    BOOL StencilEnable = FALSE,
    UINT8 StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
    UINT8 StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
    D3D12_STENCIL_OP FrontfaceStencilFailOp = D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP FrontfaceStencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP FrontfaceStencilPassOp = D3D12_STENCIL_OP_KEEP,
    D3D12_COMPARISON_FUNC FrontfaceStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
    D3D12_STENCIL_OP BackfaceStencilFailOp = D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP BackfaceStencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP BackfaceStencilPassOp = D3D12_STENCIL_OP_KEEP,
    D3D12_COMPARISON_FUNC BackfaceStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
>
class NXDepthStencilState
{
public:
    static D3D12_DEPTH_STENCIL_DESC Create()
    {
        D3D12_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = DepthEnable;
        desc.DepthWriteMask = DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = DepthFunc;
        desc.StencilEnable = StencilEnable;
        desc.StencilReadMask = StencilReadMask;
        desc.StencilWriteMask = StencilWriteMask;
        // Front face stencil ops
        desc.FrontFace.StencilFailOp = FrontfaceStencilFailOp;
        desc.FrontFace.StencilDepthFailOp = FrontfaceStencilDepthFailOp;
        desc.FrontFace.StencilPassOp = FrontfaceStencilPassOp;
        desc.FrontFace.StencilFunc = FrontfaceStencilFunc;
        // Back face stencil ops
        desc.BackFace.StencilFailOp = BackfaceStencilFailOp;
        desc.BackFace.StencilDepthFailOp = BackfaceStencilDepthFailOp;
        desc.BackFace.StencilPassOp = BackfaceStencilPassOp;
        desc.BackFace.StencilFunc = BackfaceStencilFunc;

        return desc;
    }
};

template<
    BOOL AlphaToCoverageEnable = FALSE,
    BOOL IndependentBlendEnable = TRUE,

    // RT0 settings
    BOOL RT0BlendEnable = FALSE,
    BOOL RT0LogicOpEnable = FALSE,
    D3D12_BLEND RT0SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT0DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT0BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT0SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT0DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT0BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT0LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT0RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT1 settings
    BOOL RT1BlendEnable = FALSE,
    BOOL RT1LogicOpEnable = FALSE,
    D3D12_BLEND RT1SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT1DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT1BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT1SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT1DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT1BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT1LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT1RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT2 settings
    BOOL RT2BlendEnable = FALSE,
    BOOL RT2LogicOpEnable = FALSE,
    D3D12_BLEND RT2SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT2DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT2BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT2SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT2DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT2BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT2LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT2RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT3 settings
    BOOL RT3BlendEnable = FALSE,
    BOOL RT3LogicOpEnable = FALSE,
    D3D12_BLEND RT3SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT3DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT3BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT3SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT3DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT3BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT3LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT3RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT4 settings
    BOOL RT4BlendEnable = FALSE,
    BOOL RT4LogicOpEnable = FALSE,
    D3D12_BLEND RT4SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT4DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT4BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT4SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT4DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT4BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT4LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT4RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT5 settings
    BOOL RT5BlendEnable = FALSE,
    BOOL RT5LogicOpEnable = FALSE,
    D3D12_BLEND RT5SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT5DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT5BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT5SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT5DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT5BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT5LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT5RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT6 settings
    BOOL RT6BlendEnable = FALSE,
    BOOL RT6LogicOpEnable = FALSE,
    D3D12_BLEND RT6SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT6DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT6BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT6SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT6DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT6BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT6LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT6RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,

    // RT7 settings
    BOOL RT7BlendEnable = FALSE,
    BOOL RT7LogicOpEnable = FALSE,
    D3D12_BLEND RT7SrcBlend = D3D12_BLEND_ONE,
    D3D12_BLEND RT7DestBlend = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT7BlendOp = D3D12_BLEND_OP_ADD,
    D3D12_BLEND RT7SrcBlendAlpha = D3D12_BLEND_ONE,
    D3D12_BLEND RT7DestBlendAlpha = D3D12_BLEND_ZERO,
    D3D12_BLEND_OP RT7BlendOpAlpha = D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP RT7LogicOp = D3D12_LOGIC_OP_CLEAR,
    UINT8 RT7RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
>
class NXBlendState
{
public:
    static D3D12_BLEND_DESC Create()
    {
        D3D12_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = AlphaToCoverageEnable;
        desc.IndependentBlendEnable = IndependentBlendEnable;

        // Initialize each render target blend description
        desc.RenderTarget[0] = { RT0BlendEnable, RT0LogicOpEnable, RT0SrcBlend, RT0DestBlend, RT0BlendOp, RT0SrcBlendAlpha, RT0DestBlendAlpha, RT0BlendOpAlpha, RT0LogicOp, RT0RenderTargetWriteMask };
        desc.RenderTarget[1] = { RT1BlendEnable, RT1LogicOpEnable, RT1SrcBlend, RT1DestBlend, RT1BlendOp, RT1SrcBlendAlpha, RT1DestBlendAlpha, RT1BlendOpAlpha, RT1LogicOp, RT1RenderTargetWriteMask };
        desc.RenderTarget[2] = { RT2BlendEnable, RT2LogicOpEnable, RT2SrcBlend, RT2DestBlend, RT2BlendOp, RT2SrcBlendAlpha, RT2DestBlendAlpha, RT2BlendOpAlpha, RT2LogicOp, RT2RenderTargetWriteMask };
        desc.RenderTarget[3] = { RT3BlendEnable, RT3LogicOpEnable, RT3SrcBlend, RT3DestBlend, RT3BlendOp, RT3SrcBlendAlpha, RT3DestBlendAlpha, RT3BlendOpAlpha, RT3LogicOp, RT3RenderTargetWriteMask };
        desc.RenderTarget[4] = { RT4BlendEnable, RT4LogicOpEnable, RT4SrcBlend, RT4DestBlend, RT4BlendOp, RT4SrcBlendAlpha, RT4DestBlendAlpha, RT4BlendOpAlpha, RT4LogicOp, RT4RenderTargetWriteMask };
        desc.RenderTarget[5] = { RT5BlendEnable, RT5LogicOpEnable, RT5SrcBlend, RT5DestBlend, RT5BlendOp, RT5SrcBlendAlpha, RT5DestBlendAlpha, RT5BlendOpAlpha, RT5LogicOp, RT5RenderTargetWriteMask };
        desc.RenderTarget[6] = { RT6BlendEnable, RT6LogicOpEnable, RT6SrcBlend, RT6DestBlend, RT6BlendOp, RT6SrcBlendAlpha, RT6DestBlendAlpha, RT6BlendOpAlpha, RT6LogicOp, RT6RenderTargetWriteMask };
        desc.RenderTarget[7] = { RT7BlendEnable, RT7LogicOpEnable, RT7SrcBlend, RT7DestBlend, RT7BlendOp, RT7SrcBlendAlpha, RT7DestBlendAlpha, RT7BlendOpAlpha, RT7LogicOp, RT7RenderTargetWriteMask };

        return desc;
    }
};

template<
    D3D12_FILL_MODE FillMode = D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_BACK,
    BOOL FrontCounterClockwise = FALSE,
    INT DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
    FLOAT DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    FLOAT SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    BOOL DepthClipEnable = TRUE,
    BOOL MultisampleEnable = FALSE,
    BOOL AntialiasedLineEnable = FALSE,
    UINT ForcedSampleCount = 0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
>
class NXRasterizerState
{
public:
    static D3D12_RASTERIZER_DESC Create()
    {
        D3D12_RASTERIZER_DESC desc = {};
        desc.FillMode = FillMode;
        desc.CullMode = CullMode;
        desc.FrontCounterClockwise = FrontCounterClockwise;
        desc.DepthBias = DepthBias;
        desc.DepthBiasClamp = DepthBiasClamp;
        desc.SlopeScaledDepthBias = SlopeScaledDepthBias;
        desc.DepthClipEnable = DepthClipEnable;
        desc.MultisampleEnable = MultisampleEnable;
        desc.AntialiasedLineEnable = AntialiasedLineEnable;
        desc.ForcedSampleCount = ForcedSampleCount;
        desc.ConservativeRaster = ConservativeRaster;
        return desc;
    }
};
