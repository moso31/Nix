#include "RenderStates.h"
#include "NXDXHelper.h"

ID3D11RasterizerState2*		RenderStates::WireframeRS = nullptr;
ID3D11RasterizerState2*		RenderStates::NoCullRS = nullptr;
ID3D11RasterizerState2*		RenderStates::ShadowMapRS = nullptr;

ID3D11BlendState1*			RenderStates::AlphaToCoverageBS = nullptr;
ID3D11BlendState1*			RenderStates::TransparentBS = nullptr;

ID3D11DepthStencilState*	RenderStates::CubeMapDSS = nullptr;

void RenderStates::Init()
{
	//
	// WireframeRS
	//
	D3D11_RASTERIZER_DESC2 wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC2));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	NX::ThrowIfFailed(g_pDevice->CreateRasterizerState2(&wireframeDesc, &WireframeRS));

	//
	// NoCullRS
	//
	D3D11_RASTERIZER_DESC2 noCullDesc;
	ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC2));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = false;
	noCullDesc.DepthClipEnable = true;

	NX::ThrowIfFailed(g_pDevice->CreateRasterizerState2(&noCullDesc, &NoCullRS));

	// ShadowMapRS
	D3D11_RASTERIZER_DESC2 shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_RASTERIZER_DESC2));
	shadowMapDesc.FillMode = D3D11_FILL_SOLID;
	shadowMapDesc.CullMode = D3D11_CULL_BACK;
	shadowMapDesc.FrontCounterClockwise = false;
	shadowMapDesc.DepthClipEnable = true;
	shadowMapDesc.DepthBias = 10000;
	shadowMapDesc.DepthBiasClamp = 0.0f;
	shadowMapDesc.SlopeScaledDepthBias = 2.0f;
	NX::ThrowIfFailed(g_pDevice->CreateRasterizerState2(&shadowMapDesc, &ShadowMapRS));

	//
	// AlphaToCoverageBS
	//

	D3D11_BLEND_DESC1 alphaToCoverageDesc = { 0 };
	alphaToCoverageDesc.AlphaToCoverageEnable = true;
	alphaToCoverageDesc.IndependentBlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	NX::ThrowIfFailed(g_pDevice->CreateBlendState1(&alphaToCoverageDesc, &AlphaToCoverageBS));

	//
	// TransparentBS
	//

	D3D11_BLEND_DESC1 transparentDesc = { 0 };
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	NX::ThrowIfFailed(g_pDevice->CreateBlendState1(&transparentDesc, &TransparentBS));

	// CubemapDSS
	D3D11_DEPTH_STENCIL_DESC cubeMapDesc = { 0 };
	cubeMapDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilState(&cubeMapDesc, &CubeMapDSS));
}

void RenderStates::Release()
{
	SafeReleaseCOM(WireframeRS);
	SafeReleaseCOM(NoCullRS);
	SafeReleaseCOM(AlphaToCoverageBS);
	SafeReleaseCOM(TransparentBS);
	SafeReleaseCOM(ShadowMapRS);
	SafeReleaseCOM(CubeMapDSS);
}
