#include "Renderer.h"
#include "NXTimer.h"
#include "NXGlobalBuffers.h"
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
#include "NXPSOManager.h"

#include "NXRGPassNode.h"
#include "NXRGResource.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_viewRTSize(rtSize),
	m_bEnableDebugLayer(false)
{
}

void Renderer::Init()
{
	// 输入事件
	InitEvents();

	// 初始化资源
	InitGlobalResources();

	// 全局通用纹理
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonTextures();

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

	InitRenderGraph();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	m_pRenderGraph->SetViewResolution(m_viewRTSize);
	m_pRenderGraph->Compile();

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
}

void Renderer::InitRenderGraph()
{
	m_pRenderGraph = new NXRenderGraph();
	m_pRenderGraph->SetViewResolution(m_viewRTSize);

	struct 
	{
		NXRGResource* depth;
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
	} gBufferPassData;
	NXRGPassNode* gBufferPass = new NXRGPassNode(m_pRenderGraph, "GBufferPass", new NXGBufferRenderer(m_scene));
	m_pRenderGraph->AddPass(gBufferPass, 
		[&]() {
			NXGBufferRenderer* p = (NXGBufferRenderer*)gBufferPass->GetRenderPass();
			p->SetCamera(m_scene->GetMainCamera());
		}, 
		[&](ID3D12GraphicsCommandList* pCmdList) {
			gBufferPass->ClearRT(pCmdList, gBufferPassData.depth);
			gBufferPass->ClearRT(pCmdList, gBufferPassData.rt0);
			gBufferPass->ClearRT(pCmdList, gBufferPassData.rt1);
			gBufferPass->ClearRT(pCmdList, gBufferPassData.rt2);
			gBufferPass->ClearRT(pCmdList, gBufferPassData.rt3);
		});
	gBufferPassData.rt0 = gBufferPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	gBufferPassData.rt1 = gBufferPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	gBufferPassData.rt2 = gBufferPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	gBufferPassData.rt3 = gBufferPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	gBufferPassData.depth = gBufferPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_DepthStencil });
	gBufferPassData.rt0 = gBufferPass->Write(gBufferPassData.rt0);
	gBufferPassData.rt1 = gBufferPass->Write(gBufferPassData.rt1);
	gBufferPassData.rt2 = gBufferPass->Write(gBufferPassData.rt2);
	gBufferPassData.rt3 = gBufferPass->Write(gBufferPassData.rt3);
	gBufferPassData.depth = gBufferPass->Write(gBufferPassData.depth);

	struct 
	{
		NXRGResource* depthCopy;
	} depthCopyPassData;
	NXRGPassNode* depthCopyPass = new NXRGPassNode(m_pRenderGraph, "DepthCopy", new NXDepthRenderer());
	m_pRenderGraph->AddPass(depthCopyPass, 
		[&]() {
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	depthCopyPassData.depthCopy = depthCopyPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	depthCopyPass->Read(gBufferPassData.depth);
	depthCopyPassData.depthCopy = depthCopyPass->Write(gBufferPassData.depth);

	struct 
	{
		//NXRGResource* shadowMap;
	} shadowMapPassData;
	NXRGPassNode* shadowMapPass = new NXRGPassNode(m_pRenderGraph, "ShadowMap", new NXShadowMapRenderer(m_scene));
	m_pRenderGraph->AddPass(shadowMapPass,
		[&]() {
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	// TODO: shadowMap 纹理目前直接写死在NXShadowMapRenderer，改成可配置的
	//shadowMapPassData.shadowMap = shadowMapPass->Create({ .width = 1024, .height = 1024, .format = DXGI_FORMAT_R32_TYPELESS, .handleFlags = RG_DepthStencil });
	//shadowMapPassData.shadowMap = shadowMapPass->Write(shadowMapPassData.shadowMap);
	auto pShadowMapPass = (NXShadowMapRenderer*)shadowMapPass->GetRenderPass();
	auto pCSMDepth = pShadowMapPass->GetShadowMapDepthTex(); // 现在先临时直接调用

	struct 
	{
		NXRGResource* shadowTest;
	} shadowTestPassData;
	NXRGPassNode* shadowTestPass = new NXRGPassNode(m_pRenderGraph, "ShadowTest", new NXShadowTestRenderer());
	m_pRenderGraph->AddPass(shadowTestPass,
		[&]() {
			shadowTestPass->GetRenderPass()->PushInputTex(pCSMDepth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	shadowTestPassData.shadowTest = shadowTestPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	shadowTestPass->Read(gBufferPassData.depth);
	//shadowTestPass->Read(shadowMapPassData.shadowMap);
	shadowTestPassData.shadowTest = shadowTestPass->Write(shadowTestPassData.shadowTest);

	struct 
	{
		NXRGResource* lighting;
		NXRGResource* lightingSpec;
		NXRGResource* lightingCopy;
	} litData;
	NXRGPassNode* litPass = new NXRGPassNode(m_pRenderGraph, "DeferredLighting", new NXDeferredRenderer(m_scene));
	m_pRenderGraph->AddPass(litPass,
		[&]() {
			auto pCubeMap = m_scene->GetCubeMap()->GetCubeMap();
			auto pPreFilterMap = m_scene->GetCubeMap()->GetPreFilterMap();
			auto pBRDFLut = m_pBRDFLut->GetTex();
			litPass->GetRenderPass()->PushInputTex(pCubeMap); // t6
			litPass->GetRenderPass()->PushInputTex(pPreFilterMap); // t7
			litPass->GetRenderPass()->PushInputTex(pBRDFLut); // t8
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	litData.lighting = litPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	litData.lightingSpec = litPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	litData.lightingCopy = litPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	litPass->Read(gBufferPassData.rt0); // t0
	litPass->Read(gBufferPassData.rt1); // t1
	litPass->Read(gBufferPassData.rt2); // t2
	litPass->Read(gBufferPassData.rt3); // t3
	litPass->Read(gBufferPassData.depth); // t4
	litPass->Read(shadowTestPassData.shadowTest); // t5
	litData.lighting = litPass->Write(litData.lighting);
	litData.lightingSpec = litPass->Write(litData.lightingSpec);
	litData.lightingCopy = litPass->Write(litData.lightingCopy);

	struct 
	{
		NXRGResource* buf;
	} sssData;
	NXRGPassNode* sssPass = new NXRGPassNode(m_pRenderGraph, "Subsurface", new NXSubSurfaceRenderer());
	m_pRenderGraph->AddPass(sssPass,
		[&]() {
			auto pNoise64 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64);
			sssPass->GetRenderPass()->PushInputTex(pNoise64);
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	sssData.buf = sssPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	sssPass->Read(litData.lighting);
	sssPass->Read(litData.lightingSpec);
	sssPass->Read(litData.lightingCopy);
	sssPass->Read(gBufferPassData.rt1);
	sssPass->Read(gBufferPassData.depth);
	sssData.buf = sssPass->Write(sssData.buf);

	struct
	{
		NXRGResource* buf;
		NXRGResource* depth;
	} skyPassData;
	NXRGPassNode* skyPass = new NXRGPassNode(m_pRenderGraph, "SkyLighting", new NXSkyRenderer(m_scene));
	m_pRenderGraph->AddPass(skyPass,
		[&]() {
			auto pCubeMap = m_scene->GetCubeMap()->GetCubeMap();
			skyPass->GetRenderPass()->PushInputTex(pCubeMap);
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	skyPassData.buf = skyPass->Write(sssData.buf);
	skyPassData.depth = skyPass->Write(gBufferPassData.depth);

	struct
	{
		NXRGResource* out;
	} postProcessPassData;
	NXRGPassNode* postProcessPass = new NXRGPassNode(m_pRenderGraph, "PostProcessing", new NXColorMappingRenderer());
	m_pRenderGraph->AddPass(postProcessPass,
		[&]() {
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	postProcessPassData.out = postProcessPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	postProcessPass->Read(skyPassData.buf);
	postProcessPassData.out = postProcessPass->Write(postProcessPassData.out);

	struct
	{
		NXRGResource* out;
	} debugLayerPassData;
	NXRGPassNode* debugLayerPass = new NXRGPassNode(m_pRenderGraph, "DebugLayer", new NXDebugLayerRenderer());
	m_pRenderGraph->AddPass(debugLayerPass,
		[&]() {
			auto* p = (NXDebugLayerRenderer*)debugLayerPass->GetRenderPass();
			p->OnResize(m_viewRTSize);
			shadowTestPass->GetRenderPass()->PushInputTex(pCSMDepth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	debugLayerPassData.out = debugLayerPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	debugLayerPass->Read(postProcessPassData.out);
	//debugLayerPass->Read(shadowMapPassData.shadowMap);
	debugLayerPassData.out = debugLayerPass->Write(debugLayerPassData.out);

	struct
	{
		NXRGResource* out;
	} gizmosPassData;
	NXRGPassNode* gizmosPass = new NXRGPassNode(m_pRenderGraph, "Gizmos", new NXEditorObjectRenderer(m_scene));
	m_pRenderGraph->AddPass(gizmosPass,
		[&]() {
		},
		[&](ID3D12GraphicsCommandList* pCmdList) {
		});
	gizmosPassData.out = gizmosPass->Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	gizmosPass->Read(m_bEnableDebugLayer ? debugLayerPassData.out : postProcessPassData.out);
	gizmosPassData.out = gizmosPass->Write(debugLayerPassData.out);

	m_pRenderGraph->SetPresent(gizmosPassData.out);
	m_pRenderGraph->Compile();
}
 
void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::InitGlobalResources()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		m_pCommandAllocator.Get(i) = NX12Util::CreateCommandAllocator(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		std::wstring strCmdAllocatorName(L"Main Renderer Command Allocator" + std::to_wstring(i));
		m_pCommandAllocator.Get(i)->SetName(strCmdAllocatorName.c_str());

		m_pCommandList.Get(i) = NX12Util::CreateGraphicsCommandList(NXGlobalDX::GetDevice(), m_pCommandAllocator.Get(i).Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		std::wstring strCmdListName(L"Main Renderer Command List" + std::to_wstring(i));
		m_pCommandList.Get(i)->SetName(strCmdListName.c_str());
	}

	NXAllocatorManager::GetInstance()->Init();
	NXGlobalInputLayout::Init();

	// PSOManager
	NXPSOManager::GetInstance()->Init(NXGlobalDX::GetDevice(), NXGlobalDX::GlobalCmdQueue());
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
	g_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();
}

void Renderer::RenderFrame()
{
	auto& pCommandAllocator = m_pCommandAllocator.Current();
	auto& pCommandList = m_pCommandList.Current();

	pCommandAllocator->Reset();
	pCommandList->Reset(pCommandAllocator.Get(), nullptr);

	NX12Util::BeginEvent(pCommandList.Get(), "Render Scene");

	m_pRenderGraph->Execute(pCommandList.Get());

	// 设置视口
	auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
	pCommandList->RSSetViewports(1, &vpCamera);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));

	ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
	pCommandList->SetDescriptorHeaps(1, ppHeaps);

	//m_pDepthPrepass->Render();

	// GBuffer
	m_pGBufferRenderer->Render(pCommandList.Get());

	// Depth Copy 2023.10.26
	// Burley SSS 既需要将 Depth 的模板缓存绑定到output，又需要深度信息作为input，这就会导致资源绑定冲突。
	// 所以这里 Copy 一份记录到 GBuffer 为止的 Depth 到 DepthR32。
	// 遇到需要避免资源绑定冲突的情况，就使用复制的这张作为input。
	// 【未来如有需要可升级成 Hi-Z】
	m_pDepthRenderer->Render(pCommandList.Get());

	// Shadow Map
	auto vpShadow = NX12Util::ViewPort(2048, 2048);
	pCommandList->RSSetViewports(1, &vpShadow);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpShadow));
	m_pShadowMapRenderer->Render(pCommandList.Get());
	pCommandList->RSSetViewports(1, &vpCamera);
	pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
	m_pShadowTestRenderer->Render(pCommandList.Get());

	// Deferred opaque shading
	m_pDeferredRenderer->Render(pCommandList.Get());

	// Burley SSS (2015)
	m_pSubSurfaceRenderer->Render(pCommandList.Get());

	// CubeMap
	m_pSkyRenderer->Render(pCommandList.Get());

	// Forward translucent shading
	// 2023.8.20 前向渲染暂时停用，等 3S 搞完的
	//m_pForwardRenderer->Render();
	//m_pDepthPeelingRenderer->Render(bSSSEnable);

	//// SSAO
	//m_pSSAO->Render(pSRVNormal, pSRVPosition, pSRVDepthPrepass);

	// post processing
	m_pColorMappingRenderer->Render(pCommandList.Get());

	// 绘制编辑器对象
	m_pEditorObjectRenderer->Render(pCommandList.Get());

	// 绘制调试信息层（如果有的话）
	m_pDebugLayerRenderer->Render(pCommandList.Get());

	// 设置Present RT
	m_pFinalRT = m_pRenderGraph->GetPresent();
	m_pFinalRT = bEnableDebugLayer ? NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DebugLayer) :
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);

	NX12Util::EndEvent(pCommandList.Get());

	pCommandList->Close();

	// BRDF 2D LUT 可能还没加载完，等待一下。
	WaitForBRDF2DLUTFinish();

	ID3D12CommandList* pCmdLists[] = { pCommandList.Get() };
	NXGlobalDX::GlobalCmdQueue()->ExecuteCommandLists(1, pCmdLists);

	// 更新PSOManager状态
	NXPSOManager::GetInstance()->FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pFinalRT, swapChainBuffer);
}

void Renderer::Release()
{
	SafeRelease(m_pEditorObjectRenderer);
	SafeRelease(m_pDebugLayerRenderer);
	SafeRelease(m_pGUI);

	//SafeRelease(m_pSSAO);
	//SafeDelete(m_pDepthPrepass);

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

	NXAllocatorManager::GetInstance()->Release();
	NXSubMeshGeometryEditor::GetInstance()->Release();
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

void Renderer::WaitForBRDF2DLUTFinish()
{
	NXGlobalDX::GlobalCmdQueue()->Wait(m_pBRDFLut->GlobalFence(), m_pBRDFLut->GetFenceValue());
}
