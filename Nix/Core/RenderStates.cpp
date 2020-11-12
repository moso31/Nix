#include "RenderStates.h"
#include "NXDXHelper.h"

ComPtr<ID3D11RasterizerState2>		RenderStates::WireframeRS = nullptr;
ComPtr<ID3D11RasterizerState2>		RenderStates::NoCullRS = nullptr;
ComPtr<ID3D11RasterizerState2>		RenderStates::ShadowMapRS = nullptr;

ComPtr<ID3D11BlendState1>			RenderStates::AlphaToCoverageBS = nullptr;
ComPtr<ID3D11BlendState1>			RenderStates::TransparentBS = nullptr;

ComPtr<ID3D11DepthStencilState>		RenderStates::CubeMapDSS = nullptr;

ComPtr<ID3D11SamplerState>			RenderStates::SamplerLinearWrap = nullptr;
ComPtr<ID3D11SamplerState>			RenderStates::SamplerLinearClamp = nullptr;
ComPtr<ID3D11SamplerState>			RenderStates::SamplerShadowMapPCF = nullptr;

void RenderStates::Init()
{
	InitRasterizerStates();
	InitBlendStates();
	InitDepthStencilStates();
	InitSamplerStates();
}

void RenderStates::Release()
{
}

void RenderStates::InitRasterizerStates()
{
	// WireframeRS
	D3D11_RASTERIZER_DESC2 wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC2));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	NX::ThrowIfFailed(g_pDevice->CreateRasterizerState2(&wireframeDesc, &WireframeRS));

	// NoCullRS
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
}

void RenderStates::InitBlendStates()
{
	// AlphaToCoverageBS
	D3D11_BLEND_DESC1 alphaToCoverageDesc = { 0 };
	alphaToCoverageDesc.AlphaToCoverageEnable = true;
	alphaToCoverageDesc.IndependentBlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	NX::ThrowIfFailed(g_pDevice->CreateBlendState1(&alphaToCoverageDesc, &AlphaToCoverageBS));

	// TransparentBS
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
}

void RenderStates::InitDepthStencilStates()
{
	// CubemapDSS
	D3D11_DEPTH_STENCIL_DESC cubeMapDesc = { 0 };
	cubeMapDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilState(&cubeMapDesc, &CubeMapDSS));
}

void RenderStates::InitSamplerStates()
{	
	// Create Sampler
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &SamplerLinearWrap));

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &SamplerLinearClamp));

	// shadow map 专用 PCF 滤波采样器。
	D3D11_SAMPLER_DESC sampShadowMapPCFDesc;
	ZeroMemory(&sampShadowMapPCFDesc, sizeof(sampShadowMapPCFDesc));
	sampShadowMapPCFDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampShadowMapPCFDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;	// 采用边界寻址
	sampShadowMapPCFDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampShadowMapPCFDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	for (int i = 0; i < 4; i++)
		sampShadowMapPCFDesc.BorderColor[i] = 0.0f;	// 超出边界部分为黑色
	sampShadowMapPCFDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampShadowMapPCFDesc, &SamplerShadowMapPCF));
}
