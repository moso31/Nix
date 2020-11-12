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
	static ComPtr<ID3D11DepthStencilState>	CubeMapDSS;
};