#pragma once
#include "NXRenderStates.h"

// 2023.6.27 全局 SamplerStates 类
// 一些 SamplerState 的组合非常常用，比如 PointClamp, LinearClamp, AnisotropicClamp 等等
// 这里将这些常用的 SamplerState 封装成静态全局对象，方便使用
class NXGlobalSamplerStates
{
public:
	NXGlobalSamplerStates();
	~NXGlobalSamplerStates();

	static ID3D11SamplerState* PointClamp()		{ return s_PointClamp.Get(); }
	static ID3D11SamplerState* PointWrap()		{ return s_PointWrap.Get(); }
	static ID3D11SamplerState* PointMirror()	{ return s_PointMirror.Get(); }
	static ID3D11SamplerState* LinearClamp()	{ return s_LinearClamp.Get(); }
	static ID3D11SamplerState* LinearWrap()		{ return s_LinearWrap.Get(); }
	static ID3D11SamplerState* LinearMirror()	{ return s_LinearMirror.Get(); }
	static ID3D11SamplerState* AnisoClamp()		{ return s_AnisoClamp.Get(); }
	static ID3D11SamplerState* AnisoWrap()		{ return s_AnisoWrap.Get(); }
	static ID3D11SamplerState* AnisoMirror()	{ return s_AnisoMirror.Get(); }

private:
	static ComPtr<ID3D11SamplerState> s_PointClamp;
	static ComPtr<ID3D11SamplerState> s_PointWrap;
	static ComPtr<ID3D11SamplerState> s_PointMirror;
	static ComPtr<ID3D11SamplerState> s_LinearClamp;
	static ComPtr<ID3D11SamplerState> s_LinearWrap;
	static ComPtr<ID3D11SamplerState> s_LinearMirror;
	static ComPtr<ID3D11SamplerState> s_AnisoClamp;
	static ComPtr<ID3D11SamplerState> s_AnisoWrap;
	static ComPtr<ID3D11SamplerState> s_AnisoMirror;
};
