#pragma once
#include "BaseDefs/DX12.h"

template<
    D3D12_FILTER Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
    FLOAT MipLODBias = 0.0f,
    UINT MaxAnisotropy = 1,
    D3D12_STATIC_BORDER_COLOR BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
    FLOAT MinLOD = -3.402823466e+38F, // Use D3D12_FLOAT32_MIN for clarity if available
    FLOAT MaxLOD = 3.402823466e+38F // Use D3D12_FLOAT32_MAX for clarity if available
>
class NXStaticSamplerState
{
public:
    static D3D12_STATIC_SAMPLER_DESC Create(UINT Slot = 0, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        D3D12_STATIC_SAMPLER_DESC desc;
        desc.ShaderRegister = Slot;
        desc.RegisterSpace = Space;
        desc.ShaderVisibility = Visibility;
        desc.Filter = Filter;
        desc.AddressU = AddressU;
        desc.AddressV = AddressV;
        desc.AddressW = AddressW;
        desc.MipLODBias = MipLODBias;
        desc.MaxAnisotropy = MaxAnisotropy;
        desc.ComparisonFunc = ComparisonFunc;
        desc.BorderColor = BorderColor;
        desc.MinLOD = MinLOD;
        desc.MaxLOD = MaxLOD;

        desc.SamplerFeedbackMipRegion.Width = 0;
        desc.SamplerFeedbackMipRegion.Height = 0;
        desc.SamplerFeedbackMipRegion.Depth = 0;

        return desc;
    }
};

template<
    D3D12_FILTER Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    D3D12_TEXTURE_ADDRESS_MODE AddressUVW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
    FLOAT MipLODBias = 0.0f,
    UINT MaxAnisotropy = 1,
    D3D12_STATIC_BORDER_COLOR BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
    FLOAT MinLOD = -3.402823466e+38F, // Use D3D12_FLOAT32_MIN for clarity if available
    FLOAT MaxLOD = 3.402823466e+38F // Use D3D12_FLOAT32_MAX for clarity if available
>
class NXStaticSamplerStateUVW
{
public:
    static D3D12_STATIC_SAMPLER_DESC Create(UINT Slot = 0, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        D3D12_STATIC_SAMPLER_DESC desc;
        desc.ShaderRegister = Slot;
        desc.RegisterSpace = Space;
        desc.ShaderVisibility = Visibility;
        desc.Filter = Filter;
        desc.AddressU = AddressUVW;
        desc.AddressV = AddressUVW;
        desc.AddressW = AddressUVW;
        desc.MipLODBias = MipLODBias;
        desc.MaxAnisotropy = MaxAnisotropy;
        desc.ComparisonFunc = ComparisonFunc;
        desc.BorderColor = BorderColor;
        desc.MinLOD = MinLOD;
        desc.MaxLOD = MaxLOD;

        desc.SamplerFeedbackMipRegion.Width = 0;
        desc.SamplerFeedbackMipRegion.Height = 0;
        desc.SamplerFeedbackMipRegion.Depth = 0;

        return desc;
    }
};
