#pragma once
#include "Header.h"

class RenderStates
{
public:
	static void Init();
	static void Release();

	static ComPtr<ID3D11RasterizerState2>	WireframeRS;
	static ComPtr<ID3D11RasterizerState2>	NoCullRS;
	static ComPtr<ID3D11RasterizerState2>	ShadowMapRS;

	static ComPtr<ID3D11BlendState1>		AlphaToCoverageBS;
	static ComPtr<ID3D11BlendState1>		TransparentBS;

	static ComPtr<ID3D11DepthStencilState>	DSSCubeMap;
	static ComPtr<ID3D11DepthStencilState>	DSSForwardRendering;
	static ComPtr<ID3D11DepthStencilState>	DSSDeferredRendering;

	static ComPtr<ID3D11SamplerState>		SamplerLinearWrap;
	static ComPtr<ID3D11SamplerState>		SamplerLinearClamp;
	static ComPtr<ID3D11SamplerState>		SamplerShadowMapPCF;	// shadowMap PCF滤波采样，模糊阴影边缘

private:
	static void InitRasterizerStates();
	static void InitBlendStates();
	static void InitDepthStencilStates();
	static void InitSamplerStates();
};