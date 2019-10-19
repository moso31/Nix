#include "Renderer.h"
#include "DirectResources.h"
#include "GlobalBufferManager.h"
#include "ShaderComplier.h"
#include "RenderStates.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
#include "NXPassShadowMap.h"

void Renderer::Init()
{
	InitRenderer();

	m_globalBufferManager = make_shared<NXGlobalBufferManager>();
	m_globalBufferManager->Init();

	m_scene = make_shared<Scene>();
	m_scene->Init();

	m_pPassShadowMap = make_shared<NXPassShadowMap>(m_scene);
	m_pPassShadowMap->Init(2048, 2048);

	ConstantBufferShadowMapTransform cb;
	m_scene->GetShadowMapTransformInfo(cb);
	m_pPassShadowMap->SetConstantBufferTransform(cb);
}

void Renderer::InitRenderer()
{
	// create VS & IL
	ID3DBlob* pVSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "VS", "vs_5_0", &pVSBlob), 
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\RenderTarget.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderOffScreen));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderShadowMap));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout));
	pVSBlob->Release();

	g_pContext->IASetInputLayout(m_pInputLayout);

	// Create PS
	ID3DBlob* pPSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\RenderTarget.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderOffScreen));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderShadowMap));
	pPSBlob->Release();

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
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinearWrap));

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinearClamp));

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
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampShadowMapPCFDesc, &m_pSamplerShadowMapPCF));

	// Create RenderTarget
	m_renderTarget = make_shared<NXRenderTarget>();
	{
		m_renderTarget->Init();
	}

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//g_pContext->RSSetState(nullptr);	// back culling
	//g_pContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}

void Renderer::UpdateSceneData()
{
	m_scene->UpdateTransform();
	m_scene->UpdateScripts();
}

void Renderer::DrawShadowMap()
{
	g_pContext->VSSetShader(m_pVertexShaderShadowMap, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderShadowMap, nullptr, 0);
	g_pContext->PSSetSamplers(0, 1, &m_pSamplerLinearWrap);

	g_pContext->RSSetState(RenderStates::ShadowMapRS);
	m_pPassShadowMap->Load();
	m_pPassShadowMap->UpdateConstantBuffer();
	m_pPassShadowMap->Render();
}

void Renderer::DrawScene()
{
	g_pContext->RSSetState(nullptr);	// back culling
	auto pOffScreenRTV = g_dxResources->GetOffScreenRTV();
	auto pRenderTargetView = g_dxResources->GetRenderTargetView();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();

	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));
	
	g_pContext->OMSetRenderTargets(1, &pOffScreenRTV, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pOffScreenRTV, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader, nullptr, 0);
	g_pContext->PSSetSamplers(1, 1, &m_pSamplerShadowMapPCF);

	m_scene->UpdateCamera();
	g_pContext->VSSetConstantBuffers(1, 1, &NXGlobalBufferManager::m_cbCamera);
	g_pContext->PSSetConstantBuffers(1, 1, &NXGlobalBufferManager::m_cbCamera);

	auto pCbLights = m_scene->GetConstantBufferLights();
	g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	auto pShadowMapSRV = m_pPassShadowMap->GetSRV();
	g_pContext->PSSetShaderResources(1, 1, &pShadowMapSRV);

	auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
	g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);

	auto pPrims = m_scene->GetPrimitives();
	for (auto it = pPrims.begin(); it != pPrims.end(); it++)
	{
		auto p = *it;
		p->Update();
		auto pTexSRV = p->GetTextureSRV();
		auto pMaterial = p->GetMaterialBuffer();
		g_pContext->VSSetConstantBuffers(0, 1, &NXGlobalBufferManager::m_cbWorld);
		g_pContext->PSSetShaderResources(0, 1, &pTexSRV);
		g_pContext->PSSetConstantBuffers(3, 1, &pMaterial);
		p->Render();
	}

	g_pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pRenderTargetView, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->VSSetShader(m_pVertexShaderOffScreen, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderOffScreen, nullptr, 0);

	m_renderTarget->Render();

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[2] = { nullptr };
	g_pContext->PSSetShaderResources(0, 2, pNullSRV);

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(1, 0, &parameters));
}

void Renderer::Release()
{
	if (m_pInputLayout)			m_pInputLayout->Release();
	if (m_pVertexShader)		m_pVertexShader->Release();
	if (m_pPixelShader)			m_pPixelShader->Release();
	if (m_pSamplerLinearWrap)	m_pSamplerLinearWrap->Release();
	if (m_pSamplerLinearClamp)	m_pSamplerLinearClamp->Release();
	if (m_pSamplerShadowMapPCF)	m_pSamplerShadowMapPCF->Release();

	if (m_scene)
		m_scene->Release();
	if (m_renderTarget)	
		m_renderTarget->Release();

	m_scene.reset();
	m_renderTarget.reset();
}
