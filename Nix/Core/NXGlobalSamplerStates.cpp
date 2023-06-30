#include "NXGlobalSamplerStates.h"

ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_PointClamp;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_PointWrap;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_PointMirror;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_LinearClamp;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_LinearWrap;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_LinearMirror;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_AnisoClamp;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_AnisoWrap;
ComPtr<ID3D11SamplerState> NXGlobalSamplerStates::s_AnisoMirror;

NXGlobalSamplerStates::NXGlobalSamplerStates()
{
	s_PointClamp  = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT  , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  >::Create();
	s_PointWrap   = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT  , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   >::Create();
	s_PointMirror = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT  , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR >::Create();
	s_LinearClamp = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  >::Create();
	s_LinearWrap  = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   >::Create();
	s_LinearMirror= NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR >::Create();
	s_AnisoClamp  = NXSamplerState<D3D11_FILTER_ANISOTROPIC        , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  , D3D11_TEXTURE_ADDRESS_CLAMP  >::Create();
	s_AnisoWrap   = NXSamplerState<D3D11_FILTER_ANISOTROPIC        , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   , D3D11_TEXTURE_ADDRESS_WRAP   >::Create();
	s_AnisoMirror = NXSamplerState<D3D11_FILTER_ANISOTROPIC        , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR , D3D11_TEXTURE_ADDRESS_MIRROR >::Create();
}

NXGlobalSamplerStates::~NXGlobalSamplerStates()
{
}
