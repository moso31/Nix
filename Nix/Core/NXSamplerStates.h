#pragma once
#include "BaseDefs/NixCore.h"
#include "Global.h"
#include "NXRenderStates.h"
#include "NXShaderDefinitions.h"

class NXSamplerManager;
class NXSamplerState
{
    friend class NXSamplerManager;
private:
    static ID3D11SamplerState* Create(
        D3D11_FILTER Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_MODE AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_MODE AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_MODE AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_COMPARISON_FUNC ComparisonFunc = D3D11_COMPARISON_NEVER,
        FLOAT MipLODBias = 0.0f,
        UINT MaxAnisotropy = 1,
        const Vector4 BorderColor = Vector4(1.0f),
        FLOAT MinLOD = -3.402823466e+38F,
        FLOAT MaxLOD = 3.402823466e+38F)
    {
        ID3D11SamplerState* pSamplerState = nullptr;

        D3D11_SAMPLER_DESC desc;
        desc.Filter = Filter;
        desc.AddressU = AddressU;
        desc.AddressV = AddressV;
        desc.AddressW = AddressW;
        desc.MipLODBias = MipLODBias;
        desc.MaxAnisotropy = MaxAnisotropy;
        desc.ComparisonFunc = ComparisonFunc;
        desc.BorderColor[0] = BorderColor.x;
        desc.BorderColor[1] = BorderColor.y;
        desc.BorderColor[2] = BorderColor.z;
        desc.BorderColor[3] = BorderColor.w;
        desc.MinLOD = MinLOD;
        desc.MaxLOD = MaxLOD;

        NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&desc, &pSamplerState));
        return pSamplerState;
    }
};

// 2023.6.27 全局 SamplerStates 类
// 一些 SamplerState 的组合非常常用，比如 PointClamp, LinearClamp, AnisotropicClamp 等等
// 这里将这些常用的 SamplerState 封装成静态全局对象，方便使用
class NXSamplerManager
{
public:
    NXSamplerManager() {}
	~NXSamplerManager() {}

    static void Init();

	static ID3D11SamplerState* Get(NXSamplerFilter filter, NXSamplerAddressMode addr);
	static ID3D11SamplerState* Get(NXSamplerFilter filter, NXSamplerAddressMode addrU, NXSamplerAddressMode addrV, NXSamplerAddressMode addrW);

private:
	const static int ADDRESS_TYPE_COUNT = 5; // 5 * 5 * 5 * 3 = 375 samplers.
	static D3D12_STATIC_SAMPLER_DESC s_Point[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];
	static D3D12_STATIC_SAMPLER_DESC s_Linear[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];
	static D3D12_STATIC_SAMPLER_DESC s_Aniso[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];
};
