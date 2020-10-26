#pragma once
#include "Header.h"

class RenderStates
{
public:
	static void Init();
	static void Release();

	static ID3D11RasterizerState2*	WireframeRS;
	static ID3D11RasterizerState2*	NoCullRS;
	static ID3D11RasterizerState2*	ShadowMapRS;
	static ID3D11BlendState1*		AlphaToCoverageBS;
	static ID3D11BlendState1*		TransparentBS;
	static ID3D11DepthStencilState* CubeMapDSS;
};