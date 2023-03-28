#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXEvent.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXGUI.h"

#include "NXScene.h"
#include "SceneManager.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"

Renderer::Renderer() : 
	m_bRenderGUI(true)
{
}

void Renderer::Init()
{
	// 输入事件
	InitEvents();

	NXGlobalInputLayout::Init();
	NXGlobalBufferManager::Init();

	// 渲染器
	InitRenderer();

	m_scene = new NXScene();
	SceneManager::GetInstance()->SetWorkingScene(m_scene);
	m_scene->Init();

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->GenerateBRDFLUT();

	m_pDepthPrepass = new NXDepthPrepass(m_scene);
	m_pDepthPrepass->Init(g_dxResources->GetViewSize());

	m_pGBufferRenderer = new NXGBufferRenderer(m_scene);
	m_pGBufferRenderer->Init();

	m_pSSAO = new NXSimpleSSAO();
	m_pSSAO->Init(g_dxResources->GetViewSize());

	m_pShadowMapRenderer = new NXShadowMapRenderer(m_scene);
	m_pShadowMapRenderer->Init();

	m_pShadowTestRenderer = new NXShadowTestRenderer();
	m_pShadowTestRenderer->Init();

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene, m_pBRDFLut);
	m_pDeferredRenderer->Init();

	m_pForwardRenderer = new NXForwardRenderer(m_scene, m_pBRDFLut);
	m_pForwardRenderer->Init();

	m_pDepthPeelingRenderer = new NXDepthPeelingRenderer(m_scene, m_pBRDFLut);
	m_pDepthPeelingRenderer->Init();

	m_pSkyRenderer = new NXSkyRenderer(m_scene);
	m_pSkyRenderer->Init();

	m_pColorMappingRenderer = new NXColorMappingRenderer();
	m_pColorMappingRenderer->Init();

	m_pDebugLayerRenderer = new NXDebugLayerRenderer(m_pShadowMapRenderer);
	m_pDebugLayerRenderer->Init();

	m_pFinalRenderer = new NXFinalRenderer();
	m_pFinalRenderer->Init();

	m_pEditorObjectRenderer = new NXEditorObjectRenderer(m_scene);
	m_pEditorObjectRenderer->Init();
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	// 在这里初始化CommonRT和通用纹理。
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonRT();
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonTextures();

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->OMSetDepthStencilState(nullptr, 0); 
	g_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	g_pContext->RSSetState(nullptr);	// back culling
}

void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::ResourcesReloading()
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->Update();
}

void Renderer::PipelineReloading()
{
	// 【2022.7.3 should I use a "bDirty" to control this method?】

	// 判断 FinalRenderer 使用哪张纹理RT 作为 Input
	bool bEnableDebugLayer = m_pDebugLayerRenderer->GetEnableDebugLayer();

	m_pFinalRenderer->SetInputTexture(bEnableDebugLayer ?
		m_pDebugLayerRenderer->GetDebugLayerTex() :
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing)
	);
}

void Renderer::UpdateSceneData()
{
	// 更新场景Scripts。实际上是用Scripts控制指定物体的Transform。
	m_scene->UpdateScripts();

	// 更新Transform
	m_scene->UpdateTransform();
	m_scene->UpdateTransformOfEditorObjects();

	// 更新Camera的常量缓存数据（VP矩阵、眼睛位置）
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

	// 设置视口
	auto vp = g_dxResources->GetViewPortSize();
	CD3D11_VIEWPORT vpCamera(0.0f, 0.0f, vp.x, vp.y);
	g_pContext->RSSetViewports(1, &vpCamera);

	NXTexture2D* pSceneRT = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_MainScene);
	g_pContext->ClearRenderTargetView(pSceneRT->GetRTV(), Colors::Black);

	//m_pDepthPrepass->Render();

	// GBuffer
	m_pGBufferRenderer->Render();

	// Shadow Map
	CD3D11_VIEWPORT vpShadow(0.0f, 0.0f, 2048, 2048);
	g_pContext->RSSetViewports(1, &vpShadow);
	m_pShadowMapRenderer->Render();
	g_pContext->RSSetViewports(1, &vpCamera);
	m_pShadowTestRenderer->Render(m_pShadowMapRenderer->GetShadowMapDepthTex());

	// Deferred opaque shading
	m_pDeferredRenderer->Render();

	// CubeMap
	m_pSkyRenderer->Render();

	// Forward translucent shading
	//m_pForwardRenderer->Render();
	m_pDepthPeelingRenderer->Render();

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// post processing
	m_pColorMappingRenderer->Render();

	// 绘制编辑器对象
	m_pEditorObjectRenderer->Render();

	// 绘制调试信息层（如果有的话）
	m_pDebugLayerRenderer->Render();

	// 绘制主渲染屏幕RTV：
	m_pFinalRenderer->Render();

	g_pUDA->EndEvent();
}

void Renderer::RenderGUI()
{
	if (m_bRenderGUI) m_pGUI->Render();
}

void Renderer::Release()
{
	SafeRelease(m_pEditorObjectRenderer);
	SafeRelease(m_pDebugLayerRenderer);
	SafeRelease(m_pGUI);

	SafeRelease(m_pSSAO);
	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_pGBufferRenderer);
	SafeRelease(m_pShadowMapRenderer);
	SafeRelease(m_pShadowTestRenderer);
	SafeRelease(m_pDeferredRenderer);
	SafeRelease(m_pForwardRenderer);
	SafeRelease(m_pDepthPeelingRenderer);
	SafeRelease(m_pSkyRenderer);
	SafeRelease(m_pColorMappingRenderer);
	SafeRelease(m_pFinalRenderer);

	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::OnKeyDown(NXEventArgKey eArg)
{
	if (eArg.VKey == 'H')
	{
		m_bRenderGUI = !m_bRenderGUI;
	}
}
