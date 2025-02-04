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
	m_viewRTSize(rtSize)
{
}

void Renderer::Init()
{
	// 输入事件
	InitEvents();

	// 初始化资源
	InitGlobalResources();

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
	m_pGBufferRenderer->SetCamera(m_scene->GetMainCamera());

	//m_pSSAO = new NXSimpleSSAO();
	//m_pSSAO->Init();

	m_pShadowMapRenderer = new NXShadowMapRenderer(m_scene);
	m_pShadowMapRenderer->Init();

	m_pShadowTestRenderer = new NXShadowTestRenderer();
	m_pShadowTestRenderer->SetPassName("Shadow Test");
	m_pShadowTestRenderer->RegisterTextures(2, 1);
	m_pShadowTestRenderer->SetInputTex(0, NXCommonRT_DepthZ);
	m_pShadowTestRenderer->SetInputTex(1, m_pShadowMapRenderer->GetShadowMapDepthTex());
	m_pShadowTestRenderer->SetOutputRT(0, NXCommonRT_ShadowTest);
	m_pShadowTestRenderer->SetShaderFilePath("Shader\\ShadowTest.fx");
	m_pShadowTestRenderer->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create());
	m_pShadowTestRenderer->SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
	m_pShadowTestRenderer->SetRootParams(3, 2); 
	m_pShadowTestRenderer->SetStaticRootParamCBV(0, &g_cbObject.GetFrameGPUAddresses());
	m_pShadowTestRenderer->SetStaticRootParamCBV(1, &g_cbCamera.GetFrameGPUAddresses());
	m_pShadowTestRenderer->SetStaticRootParamCBV(2, &g_cbShadowTest.GetFrameGPUAddresses());
	m_pShadowTestRenderer->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	m_pShadowTestRenderer->InitPSO();

	m_pDeferredRenderer = new NXDeferredRenderer(m_scene, m_pBRDFLut);
	m_pDeferredRenderer->Init();

	m_pSubSurfaceRenderer = new NXSubSurfaceRenderer();
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
	m_pDebugLayerRenderer->Init();

	m_pEditorObjectRenderer = new NXEditorObjectRenderer(m_scene);
	m_pEditorObjectRenderer->Init();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	NXResourceManager::GetInstance()->GetTextureManager()->ResizeCommonRT(m_viewRTSize);
	m_pDeferredRenderer->OnResize();
	m_pDepthRenderer->OnResize();
	m_pSubSurfaceRenderer->OnResize();
	m_pSkyRenderer->OnResize();
	//m_pDepthPrepass->OnResize(m_viewRTSize);
	//m_pSSAO->OnResize(m_viewRTSize);
	//m_pDepthPeelingRenderer->OnResize(m_viewRTSize);
	m_pShadowTestRenderer->OnResize();
	m_pColorMappingRenderer->OnResize();
	m_pDebugLayerRenderer->OnResize(m_viewRTSize);
	m_pEditorObjectRenderer->OnResize();

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

void Renderer::InitRenderGraph()
{
	struct 
	{
		NXRGResource* depth;
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
	} gBufferPassData;
	NXRGPassNode* gBufferPass = new NXRGPassNode(m_pRenderGraph, new NXGBufferRenderer(m_scene));
	m_pRenderGraph->AddPass(gBufferPass, 
		[&]() {}, 
		[&]() {
			gBufferPass->GetRenderPass()->Render(m_pCommandList.Current().Get());
		});
	gBufferPassData.rt0 = gBufferPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	gBufferPassData.rt1 = gBufferPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	gBufferPassData.rt2 = gBufferPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	gBufferPassData.rt3 = gBufferPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	gBufferPassData.depth = gBufferPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_DepthStencil, .handleType = RG_Texture2D });
	gBufferPassData.rt0 = gBufferPass->Write(gBufferPassData.rt0);
	gBufferPassData.rt1 = gBufferPass->Write(gBufferPassData.rt1);
	gBufferPassData.rt2 = gBufferPass->Write(gBufferPassData.rt2);
	gBufferPassData.rt3 = gBufferPass->Write(gBufferPassData.rt3);
	gBufferPassData.depth = gBufferPass->Write(gBufferPassData.depth);

	struct 
	{
		NXRGResource* depth;
	} depthCopyPassData;
	NXRGPassNode* depthCopyPass = new NXRGPassNode(m_pRenderGraph, new NXDepthRenderer());
	m_pRenderGraph->AddPass(depthCopyPass, 
		[&]() {
			auto p = (NXDepthRenderer*)(depthCopyPass->GetRenderPass());
			p->Init(); //【TODO：肯定不行，按照这次修改目前里面的资源应该已经没了。得想想办法】
		},
		[&]() {
			auto p = (NXDepthRenderer*)(depthCopyPass->GetRenderPass());
			p->Render();
		});
	depthCopyPassData.depth = depthCopyPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	depthCopyPassData.depth = depthCopyPass->Read(gBufferPassData.depth);
	depthCopyPassData.depth = depthCopyPass->Write(gBufferPassData.depth);

	struct 
	{
		NXRGResource* shadowMap;
	} shadowMapPassData;
	NXRGPassNode* shadowMapPass = new NXRGPassNode(m_pRenderGraph, new NXShadowMapRenderer(m_scene));
	m_pRenderGraph->AddPass(shadowMapPass);
	shadowMapPassData.shadowMap = shadowMapPass->Create({ .width = 1024, .height = 1024, .format = DXGI_FORMAT_R32_TYPELESS, .handleFlags = RG_DepthStencil, .handleType = RG_Texture2D });
	shadowMapPassData.shadowMap = shadowMapPass->Write(shadowMapPassData.shadowMap);

	struct 
	{
		NXRGResource* shadowMap;
		NXRGResource* depth;
		NXRGResource* shadowTest;
	} shadowTestPassData;
	NXRGPassNode* shadowTestPass = new NXRGPassNode(m_pRenderGraph, new NXShadowTestRenderer());
	m_pRenderGraph->AddPass(shadowTestPass);
	shadowTestPassData.shadowTest = shadowTestPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	shadowTestPassData.shadowMap = shadowTestPass->Read(shadowMapPassData.shadowMap);
	shadowTestPassData.depth = shadowTestPass->Read(gBufferPassData.depth);
	shadowTestPassData.shadowTest = shadowTestPass->Write(shadowTestPassData.shadowTest);

	struct 
	{
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
		NXRGResource* depth;
		NXRGResource* shadowTest;
		NXRGResource* lighting;
		NXRGResource* lightingSpec;
		NXRGResource* lightingCopy;
		NXRGResource* lightingNoUse;
	} litData;
	NXRGPassNode* litPass = new NXRGPassNode(m_pRenderGraph, new NXDeferredRenderer(m_scene));
	m_pRenderGraph->AddPass(litPass);
	litData.lighting = litPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	litData.lightingSpec = litPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	litData.lightingCopy = litPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	litData.lightingNoUse = litPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	litData.rt0 = litPass->Read(gBufferPassData.rt0);
	litData.rt1 = litPass->Read(gBufferPassData.rt1);
	litData.rt2 = litPass->Read(gBufferPassData.rt2);
	litData.rt3 = litPass->Read(gBufferPassData.rt3);
	litData.depth = litPass->Read(gBufferPassData.depth);
	// todo BRDFLUT, skymap...
	litData.shadowTest = litPass->Read(shadowTestPassData.shadowTest);
	litData.lighting = litPass->Write(litData.lighting);
	litData.lightingSpec = litPass->Write(litData.lightingSpec);
	litData.lightingCopy = litPass->Write(litData.lightingCopy);
	litData.lightingNoUse = litPass->Write(litData.lightingNoUse);

	struct 
	{
		NXRGResource* lighting;
		NXRGResource* lightingSpec;
		NXRGResource* lightingCopy;
		NXRGResource* rt1;
		NXRGResource* depth;
		NXRGResource* buf;
	} sssData;
	NXRGPassNode* sssPass = new NXRGPassNode(m_pRenderGraph, new NXSubSurfaceRenderer());
	m_pRenderGraph->AddPass(sssPass);
	sssData.buf = sssPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	sssData.lighting = sssPass->Read(litData.lighting);
	sssData.lightingSpec = sssPass->Read(litData.lightingSpec);
	sssData.lightingCopy = sssPass->Read(litData.lightingCopy);
	sssData.rt1 = sssPass->Read(gBufferPassData.rt1);
	sssData.depth = sssPass->Read(gBufferPassData.depth);
	// todo noise64x64...
	sssData.buf = sssPass->Write(sssData.buf);

	struct
	{
		NXRGResource* buf;
		NXRGResource* depth;
	} skyPassData;
	NXRGPassNode* skyPass = new NXRGPassNode(m_pRenderGraph, new NXSkyRenderer(m_scene));
	m_pRenderGraph->AddPass(skyPass);
	// todo cube...
	skyPassData.buf = skyPass->Write(sssData.buf);
	skyPassData.depth = skyPass->Write(sssData.depth);

	struct
	{
		NXRGResource* in;
		NXRGResource* out;
	} postProcessPassData;
	NXRGPassNode* postProcessPass = new NXRGPassNode(m_pRenderGraph, new NXColorMappingRenderer());
	m_pRenderGraph->AddPass(postProcessPass);
	postProcessPassData.out = postProcessPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	postProcessPassData.in = postProcessPass->Read(skyPassData.buf);
	postProcessPassData.out = postProcessPass->Write(postProcessPassData.out);

	struct
	{
		NXRGResource* base;
		NXRGResource* csm;
		NXRGResource* out;
	} debugLayerPassData;
	NXRGPassNode* debugLayerPass = new NXRGPassNode(m_pRenderGraph, new NXDebugLayerRenderer());
	m_pRenderGraph->AddPass(debugLayerPass);
	debugLayerPassData.out = debugLayerPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	debugLayerPassData.base = debugLayerPass->Read(postProcessPassData.out);
	debugLayerPassData.csm = debugLayerPass->Read(shadowMapPassData.shadowMap);
	debugLayerPassData.out = debugLayerPass->Write(debugLayerPassData.out);

	struct
	{
		NXRGResource* in;
		NXRGResource* out;
	} gizmosPassData;
	NXRGPassNode* gizmosPass = new NXRGPassNode(m_pRenderGraph, new NXEditorObjectRenderer(m_scene));
	m_pRenderGraph->AddPass(gizmosPass);
	gizmosPassData.out = gizmosPass->Create({ .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget, .handleType = RG_Texture2D });
	gizmosPassData.in = gizmosPass->Read(debugLayerPassData.out);
	gizmosPassData.out = gizmosPass->Write(gizmosPassData.in);

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

	// 判断 GUIView 使用哪张纹理RT 作为 Input
	bool bEnableDebugLayer = m_pDebugLayerRenderer->GetEnableDebugLayer();
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
