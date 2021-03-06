#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "RenderStates.h"
#include "NXGUI.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXPassShadowMap.h"

void Renderer::Init()
{
	NXGlobalInputLayout::Init();
	NXGlobalBufferManager::Init();

	InitRenderer();

	m_scene = new NXScene();
	m_scene->Init();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->GenerateIrradianceMap();
		pCubeMap->GeneratePreFilterMap();
		pCubeMap->GenerateBRDF2DLUT();
	}

	m_pDepthPrepass = new NXDepthPrepass(m_scene);
	m_pDepthPrepass->Init(g_dxResources->GetViewSize());

	m_pSSAO = new NXSimpleSSAO();
	m_pSSAO->Init(g_dxResources->GetViewSize());

	// forward or deferred renderer?
	{
		m_pForwardRenderer = new NXForwardRenderer(m_scene);
		m_pForwardRenderer->Init();

		m_pDeferredRenderer = new NXDeferredRenderer(m_scene);
		m_pDeferredRenderer->Init();

		// 【待改】这个bool将来做成Settings（配置文件）之类的结构。
		m_isDeferredShading = 1;
	}

	m_pPassShadowMap = new NXPassShadowMap(m_scene);
	m_pPassShadowMap->Init(2048, 2048);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, m_pSSAO);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	// create VS & IL
	ComPtr<ID3DBlob> pVSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderShadowMap));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutPNT));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMap.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderCubeMap)); 

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutP));

	// Create PS
	ComPtr<ID3DBlob> pPSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\ShadowMap.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderShadowMap));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMap.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderCubeMap));

	// Create RenderTarget
	m_renderTarget = new NXRenderTarget();
	m_renderTarget->Init();

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->RSSetState(nullptr);	// back culling
	g_pContext->OMSetDepthStencilState(nullptr, 0); 

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());
}

void Renderer::UpdateSceneData()
{
	// 更新场景Scripts。实际上是用Scripts控制指定物体的Transform。
	m_scene->UpdateScripts();

	// 更新Transform
	m_scene->UpdateTransform();

	// 更新Camera的常量缓存数据（VP矩阵、眼睛位置）
	m_scene->UpdateCamera();

	m_pSSAO->Update();
}

void Renderer::DrawScene()
{
	g_pUDA->BeginEvent(L"Render Scene");

	// 渲染主场景所用的Sampler
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();	// 绘制主场景的RTV
	auto pRTVFinalQuad = g_dxResources->GetRTVFinalQuad();	// 绘制最终渲染Quad的RTV

	auto pSRVDepthStencil = g_dxResources->GetSRVDepthStencil();
	auto pDSVDepthStencil = g_dxResources->GetDSVDepthStencil();

	auto pSRVPosition = m_pDepthPrepass->GetSRVPosition();
	auto pSRVNormal = m_pDepthPrepass->GetSRVNormal();
	auto pSRVDepthPrepass = m_pDepthPrepass->GetSRVDepthPrepass();
	auto pDSVDepthPrepass = m_pDepthPrepass->GetDSVDepthPrepass();

	// 设置视口
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));

	// DepthPrepass
	m_pDepthPrepass->Render();

	if (!m_isDeferredShading)
	{
		// SSAO
		g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
		g_pContext->ClearRenderTargetView(pRTVMainScene, Colors::WhiteSmoke);
		g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

		// Forward shading
		m_pForwardRenderer->Render(m_pSSAO->GetSRV());
	}
	else
	{
		// Deferred shading: RenderGBuffer
		m_pDeferredRenderer->RenderGBuffer();

		// SSAO
		m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

		// Deferred shading: Render
		g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
		g_pContext->ClearRenderTargetView(pRTVMainScene, Colors::WhiteSmoke);
		g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pDeferredRenderer->Render(m_pSSAO->GetSRV());
	}

	// 绘制CubeMap
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
	g_pContext->OMSetDepthStencilState(RenderStates::DSSCubeMap.Get(), 0);
	DrawCubeMap();

	// 以上操作全部都是在主RTV中进行的。
	// 下面切换到QuadRTV，简单来说就是将主RTV绘制到这个RTV，然后作为一张四边形纹理进行最终输出。
	g_pContext->OMSetRenderTargets(1, &pRTVFinalQuad, pDSVDepthStencil);
	g_pContext->ClearRenderTargetView(pRTVFinalQuad, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->RSSetState(nullptr);
	g_pContext->OMSetDepthStencilState(nullptr, 0);

	// 绘制主渲染屏幕RTV：
	m_renderTarget->Render();

	g_pUDA->EndEvent();
}

void Renderer::DrawGUI()
{
	m_pGUI->Render();
}

void Renderer::Release()
{
	SafeRelease(m_pGUI);

	SafeDelete(m_pPassShadowMap);
	SafeDelete(m_pSSAO);

	SafeDelete(m_pDeferredRenderer);
	SafeDelete(m_pForwardRenderer);

	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::DrawCubeMap()
{
	g_pUDA->BeginEvent(L"Cube Map");
	g_pContext->IASetInputLayout(m_pInputLayoutP.Get());
	g_pContext->VSSetShader(m_pVertexShaderCubeMap.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderCubeMap.Get(), nullptr, 0);

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);

		pCubeMap->Render();
	}
	g_pUDA->EndEvent();
}

void Renderer::DrawShadowMap()
{
	g_pContext->VSSetShader(m_pVertexShaderShadowMap.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderShadowMap.Get(), nullptr, 0);
	g_pContext->PSSetSamplers(1, 1, RenderStates::SamplerShadowMapPCF.GetAddressOf());

	g_pContext->RSSetState(RenderStates::ShadowMapRS.Get());
	m_pPassShadowMap->Load();
	m_pPassShadowMap->UpdateConstantBuffer();
	m_pPassShadowMap->Render();
}