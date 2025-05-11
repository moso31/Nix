#include "NXSamplerManager.h"

const D3D12_STATIC_SAMPLER_DESC& NXSamplerManager::CreateIso(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW, D3D12_COMPARISON_FUNC comparisonFunc, FLOAT mipLODBias, UINT maxAnisotropy, D3D12_STATIC_BORDER_COLOR borderColor, FLOAT minLOD, FLOAT maxLOD)
{
    return Create(slot, space, visibility, filter, addrUVW, addrUVW, addrUVW, comparisonFunc, mipLODBias, maxAnisotropy, borderColor, minLOD, maxLOD);
}

const D3D12_STATIC_SAMPLER_DESC& NXSamplerManager::Create(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrU, D3D12_TEXTURE_ADDRESS_MODE addrV, D3D12_TEXTURE_ADDRESS_MODE addrW, D3D12_COMPARISON_FUNC comparisonFunc, FLOAT mipLODBias, UINT maxAnisotropy, D3D12_STATIC_BORDER_COLOR borderColor, FLOAT minLOD, FLOAT maxLOD)
{
    D3D12_STATIC_SAMPLER_DESC desc;
    desc.ShaderRegister     = slot;
    desc.RegisterSpace      = space;
    desc.ShaderVisibility   = visibility;
    desc.Filter             = filter;
    desc.AddressU           = addrU;
    desc.AddressV           = addrV;
    desc.AddressW           = addrW;
    desc.MipLODBias         = mipLODBias;
    desc.MaxAnisotropy      = maxAnisotropy;
    desc.ComparisonFunc     = comparisonFunc;
    desc.BorderColor        = borderColor;
    desc.MinLOD             = minLOD;
    desc.MaxLOD             = maxLOD;

    for (auto& samplerDesc : m_samplerDescs)
    {
        if (IsSame(samplerDesc, desc))
            return samplerDesc;
    }

    return m_samplerDescs.emplace_back(desc);
}

const D3D12_STATIC_SAMPLER_DESC &NXSamplerManager::Create(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility, const NXMatDataSampler* ssInfo)
{
    return Create(slot, space, visibility, ToFilterMode(ssInfo->filter), ToAddressMode(ssInfo->addressU), ToAddressMode(ssInfo->addressV), ToAddressMode(ssInfo->addressW), D3D12_COMPARISON_FUNC_NEVER, 0.0f, 1, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK, -D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX);
}

bool NXSamplerManager::IsSame(const D3D12_STATIC_SAMPLER_DESC& lhs, const D3D12_STATIC_SAMPLER_DESC& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

D3D12_TEXTURE_ADDRESS_MODE NXSamplerManager::ToAddressMode(NXSamplerAddressMode address)
{
    switch (address)
    {
    case NXSamplerAddressMode::Wrap:        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case NXSamplerAddressMode::Mirror:      return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case NXSamplerAddressMode::Clamp:       return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case NXSamplerAddressMode::Border:      return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case NXSamplerAddressMode::MirrorOnce:  return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    default:                                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

D3D12_FILTER NXSamplerManager::ToFilterMode(NXSamplerFilter filter)
{
	switch (filter)
	{
	case NXSamplerFilter::Point:
		return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	case NXSamplerFilter::Linear:
		return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	case NXSamplerFilter::Anisotropic:
		return D3D12_FILTER_ANISOTROPIC;
	case NXSamplerFilter::Unknown:
	default: 
        return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	}
}
