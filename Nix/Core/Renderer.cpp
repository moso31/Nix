#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXGUI.h"

#include "NXRenderTarget.h"
#include "NXScene.h"
#include "SceneManager.h"
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
	SceneManager::GetInstance()->SetWorkingScene(m_scene);
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

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene);
	m_pDeferredRenderer->Init();

	m_pForwardRenderer = new NXForwardRenderer(m_scene);
	m_pForwardRenderer->Init();

	m_pSkyRenderer = new NXSkyRenderer(m_scene);
	m_pSkyRenderer->Init();

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
	// �������ʼ��CommonRT��
	NXResourceManager::GetInstance()->InitCommonRT();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ShadowMap.fx", "VS", &m_pVertexShaderShadowMap, NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), &m_pInputLayoutPNT);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowMap.fx", "PS", &m_pPixelShaderShadowMap);

	// Create RenderTarget
	m_renderTarget = new NXRenderTarget();
	m_renderTarget->Init();

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->OMSetDepthStencilState(nullptr, 0); 
	g_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	g_pContext->RSSetState(nullptr);	// back culling
}

void Renderer::ResourcesReloading()
{
	NXResourceReloader::GetInstance()->Update();
}

void Renderer::UpdateSceneData()
{
	// ���³���Scripts��ʵ��������Scripts����ָ�������Transform��
	m_scene->UpdateScripts();

	// ����Transform
	m_scene->UpdateTransform();

	// ����Camera�ĳ����������ݣ�VP�����۾�λ�ã�
	m_scene->UpdateCamera();

	m_scene->UpdateLightData();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update();
	}

	m_pSSAO->Update();
}

void Renderer::RenderFrame()
{
	g_pUDA->BeginEvent(L"Render Scene");

	// �����ӿ�
	auto vp = g_dxResources->GetViewPortSize();
	g_pContext->RSSetViewports(1, &CD3D11_VIEWPORT(0.0f, 0.0f, vp.x, vp.y));

	NXTexture2D* pSceneRT = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene);
	g_pContext->ClearRenderTargetView(pSceneRT->GetRTV(), Colors::Black);

	//m_pDepthPrepass->Render();

	// Deferred shading: RenderGBuffer
	m_pDeferredRenderer->RenderGBuffer();

	// Deferred shading: Render
	m_pDeferredRenderer->Render();

	// Forward shading
	m_pForwardRenderer->Render();

	// ����CubeMap
	m_pSkyRenderer->Render();

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// ��������Ⱦ��ĻRTV��
	m_renderTarget->Render();

	g_pUDA->EndEvent();
}

void Renderer::RenderGUI()
{
	m_pGUI->Render();
}

void Renderer::Release()
{
	SafeRelease(m_pGUI);

	SafeDelete(m_pPassShadowMap);
	SafeRelease(m_pSSAO);

	SafeRelease(m_pDeferredRenderer);
	SafeRelease(m_pForwardRenderer);
	SafeRelease(m_pSkyRenderer);

	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_scene);
	SafeRelease(m_renderTarget);
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::DrawShadowMap()
{
	//g_pContext->VSSetShader(m_pVertexShaderShadowMap.Get(), nullptr, 0);
	//g_pContext->PSSetShader(m_pPixelShaderShadowMap.Get(), nullptr, 0);
	//g_pContext->PSSetSamplers(1, 1, RenderStates::SamplerShadowMapPCF.GetAddressOf());

	//g_pContext->RSSetState(RenderStates::ShadowMapRS.Get());
	//m_pPassShadowMap->Load();
	//m_pPassShadowMap->UpdateConstantBuffer();
	//m_pPassShadowMap->Render();
}