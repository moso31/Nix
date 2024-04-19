#include "Renderer.h"
#include "NXTimer.h"
#include "NXGlobalDefinitions.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXEvent.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXGUI.h"
#include "NXTexture.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_viewRTSize(rtSize)
{
}

void Renderer::Init()
{
	// 输入事件
	InitEvents();

	// 创建各种DX12资源分配器
	NXAllocatorManager::GetInstance()->Init();

	NXGlobalInputLayout::Init();
	NXGlobalBuffer::Init();

	NXTexture::Init();

	NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.GetAddressOf(), m_pCommandAllocator.GetAddressOf(), m_pCommandList.GetAddressOf());

	// 渲染器
	InitRenderer();

	NXSubMeshGeometryEditor::GetInstance()->Init(NXGlobalDX::GetDevice());

	m_scene = new NXScene();

	NXResourceManager::GetInstance()->GetMaterialManager()->Init();

	NXResourceManager::GetInstance()->GetMeshManager()->Init(m_scene);
	NXResourceManager::GetInstance()->GetCameraManager()->SetWorkingScene(m_scene);
	NXResourceManager::GetInstance()->GetLightManager()->SetWorkingScene(m_scene);

	m_scene->Init();

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->Init();

	//m_pDepthPrepass = new NXDepthPrepass(m_scene);
	//m_pDepthPrepass->Init();

	m_pDepthRenderer = new NXDepthRenderer();
	m_pDepthRenderer->Init();

	m_pGBufferRenderer = new NXGBufferRenderer(m_scene);
	m_pGBufferRenderer->Init();

	//m_pSSAO = new NXSimpleSSAO();
	//m_pSSAO->Init();

	m_pShadowMapRenderer = new NXShadowMapRenderer(m_scene);
	m_pShadowMapRenderer->Init();

	m_pShadowTestRenderer = new NXShadowTestRenderer();
	m_pShadowTestRenderer->Init();

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene, m_pBRDFLut);
	m_pDeferredRenderer->Init();

	m_pSubSurfaceRenderer = new NXSubSurfaceRenderer(m_scene);
	m_pSubSurfaceRenderer->Init();

	//m_pForwardRenderer = new NXForwardRenderer(m_scene, m_pBRDFLut);
	//m_pForwardRenderer->Init();

	//m_pDepthPeelingRenderer = new NXDepthPeelingRenderer(m_scene, m_pBRDFLut);
	//m_pDepthPeelingRenderer->Init();

	m_pSkyRenderer = new NXSkyRenderer(m_scene);
	m_pSkyRenderer->Init();

	m_pColorMappingRenderer = new NXColorMappingRenderer();
	m_pColorMappingRenderer->Init();

	m_pDebugLayerRenderer = new NXDebugLayerRenderer(m_pShadowMapRenderer);
	m_pDebugLayerRenderer->Init(m_viewRTSize);

	m_pEditorObjectRenderer = new NXEditorObjectRenderer(m_scene);
	m_pEditorObjectRenderer->Init();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	NXResourceManager::GetInstance()->GetTextureManager()->ResizeCommonRT(m_viewRTSize);
	//m_pDepthPrepass->OnResize(m_viewRTSize);
	//m_pSSAO->OnResize(m_viewRTSize);
	//m_pDepthPeelingRenderer->OnResize(m_viewRTSize);
	m_pDebugLayerRenderer->OnResize(m_viewRTSize);
	m_pEditorObjectRenderer->OnResize(m_viewRTSize);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
}

void Renderer::InitRenderer()
{
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonTextures();

	// 2024.4.9 【InitCommonRT 和 OnResize 会导致不必要的重复创建RT。需要优化】
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonRT(m_viewRTSize);
}
 
void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::ResourcesReloading()
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();
}

void Renderer::Update()
{
	UpdateGUI();
	UpdateSceneData();
}

void Renderer::UpdateGUI()
{
	m_pGUI->ExecuteDeferredCommands();
}

void Renderer::UpdateSceneData()
{
	UpdateTime();

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

	//m_pSSAO->Update();
}

void Renderer::UpdateTime()
{
	size_t globalTime = NXGlobalApp::Timer->GetGlobalTime();
	float fGlobalTime = globalTime / 1000.0f;

	NXGlobalBuffer::cbObject.Current().globalData.time = fGlobalTime;
}

void Renderer::RenderFrame()
{
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	auto& pSceneRT = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0);
	if (pSceneRT.IsNull()) return;

	NX12Util::BeginEvent(m_pCommandList.Get(), "Render Scene");

	// 设置视口
	auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
	m_pCommandList->RSSetViewports(1, &vpCamera);
	m_pCommandList->ClearRenderTargetView(pSceneRT->GetRTV(), Colors::Black, 0, nullptr);

	//m_pDepthPrepass->Render();

	// GBuffer
	m_pGBufferRenderer->Render(m_pCommandList.Get());

	// Depth Copy 2023.10.26
	// Burley SSS 既需要将 Depth 的模板缓存绑定到output，又需要深度信息作为input，这就会导致资源绑定冲突。
	// 所以这里 Copy 一份记录到 GBuffer 为止的 Depth 到 DepthR32。
	// 遇到需要避免资源绑定冲突的情况，就使用复制的这张作为input。
	// 【未来如有需要可升级成 Hi-Z】
	m_pDepthRenderer->Render(m_pCommandList.Get());

	// Shadow Map
	auto vpShadow = NX12Util::ViewPort(2048, 2048);
	m_pCommandList->RSSetViewports(1, &vpShadow);
	m_pShadowMapRenderer->Render(m_pCommandList.Get());
	m_pCommandList->RSSetViewports(1, &vpCamera);
	m_pShadowTestRenderer->SetShadowMapDepth(m_pShadowMapRenderer->GetShadowMapDepthTex());
	m_pShadowTestRenderer->Render(m_pCommandList.Get());

	// Deferred opaque shading
	m_pDeferredRenderer->Render(m_pCommandList.Get());

	// Burley SSS (2015)
	m_pSubSurfaceRenderer->Render(m_pCommandList.Get());

	// CubeMap
	m_pSkyRenderer->Render(m_pCommandList.Get());

	// Forward translucent shading
	// 2023.8.20 前向渲染暂时停用，等 3S 搞完的
	//m_pForwardRenderer->Render();
	//m_pDepthPeelingRenderer->Render(bSSSEnable);

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// post processing
	m_pColorMappingRenderer->Render(m_pCommandList.Get());

	// 绘制编辑器对象
	m_pEditorObjectRenderer->Render(m_pCommandList.Get());

	// 绘制调试信息层（如果有的话）
	m_pDebugLayerRenderer->Render(m_pCommandList.Get());

	// 判断 GUIView 使用哪张纹理RT 作为 Input
	bool bEnableDebugLayer = m_pDebugLayerRenderer->GetEnableDebugLayer();
	m_pFinalRT = bEnableDebugLayer ? m_pDebugLayerRenderer->GetDebugLayerTex() :
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);

	NX12Util::EndEvent();

	m_pCommandList->Close();
	ID3D12CommandList* pCmdLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, pCmdLists);
}

void Renderer::RenderGUI(D3D12_CPU_DESCRIPTOR_HANDLE swapChainRTV)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pFinalRT, swapChainRTV);
}

void Renderer::Release()
{
	SafeRelease(m_pEditorObjectRenderer);
	SafeRelease(m_pDebugLayerRenderer);
	SafeRelease(m_pGUI);

	//SafeRelease(m_pSSAO);
	SafeDelete(m_pDepthPrepass);

	SafeRelease(m_pDepthRenderer);
	SafeRelease(m_pGBufferRenderer);
	SafeRelease(m_pShadowMapRenderer);
	SafeRelease(m_pShadowTestRenderer);
	SafeRelease(m_pDeferredRenderer);
	SafeRelease(m_pSubSurfaceRenderer);
	//SafeRelease(m_pForwardRenderer);
	//SafeRelease(m_pDepthPeelingRenderer);
	SafeRelease(m_pSkyRenderer);
	SafeRelease(m_pColorMappingRenderer);

	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);
}

void Renderer::ClearAllPSResources()
{
}

void Renderer::DrawDepthPrepass()
{
}

void Renderer::OnKeyDown(NXEventArgKey eArg)
{
	//if (eArg.VKey == 'H')
	//{
	//	m_bRenderGUI = !m_bRenderGUI;
	//}
}
