#include "NXSamplerStates.h"

ComPtr<ID3D11SamplerState> NXSamplerManager::s_Point[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];
ComPtr<ID3D11SamplerState> NXSamplerManager::s_Linear[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];
ComPtr<ID3D11SamplerState> NXSamplerManager::s_Aniso[ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT][ADDRESS_TYPE_COUNT];

void NXSamplerManager::Init()
{
	for (int i = D3D11_TEXTURE_ADDRESS_WRAP; i <= D3D11_TEXTURE_ADDRESS_MIRROR_ONCE; i++)
	{
		for (int j = D3D11_TEXTURE_ADDRESS_WRAP; j <= D3D11_TEXTURE_ADDRESS_MIRROR_ONCE; j++)
		{
			for (int k = D3D11_TEXTURE_ADDRESS_WRAP; k <= D3D11_TEXTURE_ADDRESS_MIRROR_ONCE; k++)
			{
				D3D11_TEXTURE_ADDRESS_MODE addressModeU = (D3D11_TEXTURE_ADDRESS_MODE)(i);
				D3D11_TEXTURE_ADDRESS_MODE addressModeV = (D3D11_TEXTURE_ADDRESS_MODE)(j);
				D3D11_TEXTURE_ADDRESS_MODE addressModeW = (D3D11_TEXTURE_ADDRESS_MODE)(k);
				s_Point [i - 1][j - 1][k - 1] = NXSamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_POINT, addressModeU, addressModeV, addressModeW);
				s_Linear[i - 1][j - 1][k - 1] = NXSamplerState::Create(D3D11_FILTER_MIN_MAG_MIP_LINEAR, addressModeU, addressModeV, addressModeW);
				s_Aniso [i - 1][j - 1][k - 1] = NXSamplerState::Create(D3D11_FILTER_ANISOTROPIC, addressModeU, addressModeV, addressModeW);
			}
		}
	}
}

ID3D11SamplerState* NXSamplerManager::Get(NXSamplerFilter filter, NXSamplerAddressMode addr)
{
	int i = (int)addr;
	switch (filter)
	{
	case NXSamplerFilter::Point:
		return s_Point[i][i][i].Get();
		break;
	case NXSamplerFilter::Anisotropic:
		return s_Aniso[i][i][i].Get();
		break;
	case NXSamplerFilter::Linear:
	default:
		return s_Linear[i][i][i].Get();
		break;
	}

	return nullptr;
}

ID3D11SamplerState* NXSamplerManager::Get(NXSamplerFilter filter, NXSamplerAddressMode addrU, NXSamplerAddressMode addrV, NXSamplerAddressMode addrW)
{
	int i = (int)addrU;
	int j = (int)addrV;
	int k = (int)addrW;
	switch (filter)
	{
	case NXSamplerFilter::Point:
		return s_Point[i][j][k].Get();
		break;
	case NXSamplerFilter::Anisotropic:
		return s_Aniso[i][j][k].Get();
		break;
	case NXSamplerFilter::Linear:
	default:
		return s_Linear[i][j][k].Get();
		break;
	}
	return nullptr;
}
