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
#include "NXBuffer.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXPSOManager.h"

#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"

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
	m_pRenderGraph->Compile(true);

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

	struct GBufferData
	{
		NXRGResource* depth;
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
	};

	NXRGResource* pGBuffer0 = m_pRenderGraph->CreateResource("GBuffer RT0", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer1 = m_pRenderGraph->CreateResource("GBuffer RT1", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer2 = m_pRenderGraph->CreateResource("GBuffer RT2", { .format = DXGI_FORMAT_R10G10B10A2_UNORM, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer3 = m_pRenderGraph->CreateResource("GBuffer RT3", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	NXRGResource* pDepthZ = m_pRenderGraph->CreateResource("DepthZ", { .format = DXGI_FORMAT_R24G8_TYPELESS, .handleFlags = RG_DepthStencil });

	auto gBufferPassData = m_pRenderGraph->AddPass<GBufferData>("GBufferPass", new NXGBufferRenderer(m_scene),
		[&](NXRGBuilder& pBuilder, GBufferData& data) {
			NXGBufferRenderer* p = (NXGBufferRenderer*)pBuilder.GetPassNode()->GetRenderPass();
			p->SetCamera(m_scene->GetMainCamera()); // todo：别把绑相机写在这里吧？

			data.rt0 = pBuilder.WriteRT(pGBuffer0, 0);
			data.rt1 = pBuilder.WriteRT(pGBuffer1, 1);
			data.rt2 = pBuilder.WriteRT(pGBuffer2, 2);
			data.rt3 = pBuilder.WriteRT(pGBuffer3, 3);
			data.depth = pBuilder.WriteDS(pDepthZ);
		}, 
		[=](ID3D12GraphicsCommandList* pCmdList, GBufferData& data) {
			m_pRenderGraph->ClearRT(pCmdList, pGBuffer0);
			m_pRenderGraph->ClearRT(pCmdList, pGBuffer1);
			m_pRenderGraph->ClearRT(pCmdList, pGBuffer2);
			m_pRenderGraph->ClearRT(pCmdList, pGBuffer3);
			m_pRenderGraph->ClearRT(pCmdList, pDepthZ);
			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, m_viewRTSize); // 第一个pass设置ViewPort
		});

	struct DepthCopyData
	{
		NXRGResource* depthCopy;
	};

	NXRGResource* pDepthCopy = m_pRenderGraph->CreateResource("DepthCopy", { .format = DXGI_FORMAT_R32_FLOAT, .handleFlags = RG_RenderTarget });

	auto depthCopyPassData = 
	m_pRenderGraph->AddPass<DepthCopyData>("DepthCopy", new NXDepthRenderer(),
		[&](NXRGBuilder& builder, DepthCopyData& data) {
			builder.Read(gBufferPassData->GetData().depth, 0);
			data.depthCopy = builder.WriteRT(pDepthCopy, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DepthCopyData& data) {
		});

	struct ShadowMapData
	{
	};

	uint32_t shadowMapRTSize = 2048;
	uint32_t cascadeCount = 4;
	Ntr<NXTexture2DArray> pShadowMapDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture2DArray("Shadow DepthZ RT", DXGI_FORMAT_R32_TYPELESS, shadowMapRTSize, shadowMapRTSize, cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false);
	pShadowMapDepth->SetViews(1, 0, cascadeCount, 0);
	for (UINT i = 0; i < cascadeCount; i++) pShadowMapDepth->SetDSV(i, i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	pShadowMapDepth->SetSRV(0, 0, cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）

	auto pCSMDepth = m_pRenderGraph->ImportResource(pShadowMapDepth, RG_DepthStencil);
	auto shadowMapPassData = m_pRenderGraph->AddPass<ShadowMapData>("ShadowMap", new NXShadowMapRenderer(m_scene),
		[&](NXRGBuilder& builder, ShadowMapData& data) {
			builder.WriteDS(pCSMDepth);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, ShadowMapData& data) {
			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, Vector2((float)shadowMapRTSize)); // CSM ShadowMap 的 ViewPort
		});

	struct ShadowTestData
	{
		NXRGResource* shadowTest;
	};

	auto pShadowTest = m_pRenderGraph->CreateResource("ShadowTest RT", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });

	auto shadowTestPassData = m_pRenderGraph->AddPass<ShadowTestData>("ShadowTest", new NXShadowTestRenderer(),
		[&](NXRGBuilder& builder, ShadowTestData& data) {
			builder.Read(gBufferPassData->GetData().depth, 0); 
			builder.Read(pCSMDepth, 1);
			data.shadowTest = builder.WriteRT(pShadowTest, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, ShadowTestData& data) {
			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, m_viewRTSize);
		});

	struct DeferredLightingData
	{
		NXRGResource* lighting;
		NXRGResource* lightingSpec;
		NXRGResource* lightingCopy;
	};

	auto pLit = m_pRenderGraph->CreateResource("Lighting RT0", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	auto pLitSpec = m_pRenderGraph->CreateResource("Lighting RT1", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	auto pLitCopy = m_pRenderGraph->CreateResource("Lighting RT Copy", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });
	auto pCubeMap = m_pRenderGraph->ImportResource(m_scene->GetCubeMap()->GetCubeMap());
	auto pPreFilter = m_pRenderGraph->ImportResource(m_scene->GetCubeMap()->GetPreFilterMap());
	auto pBRDFLut = m_pRenderGraph->ImportResource(m_pBRDFLut->GetTex());

	auto litPassData = m_pRenderGraph->AddPass<DeferredLightingData>("DeferredLighting", new NXDeferredRenderer(m_scene),
		[&](NXRGBuilder& builder, DeferredLightingData& data) {
			builder.Read(gBufferPassData->GetData().rt0, 0); // register t0
			builder.Read(gBufferPassData->GetData().rt1, 1); // t1
			builder.Read(gBufferPassData->GetData().rt2, 2); // t2
			builder.Read(gBufferPassData->GetData().rt3, 3); // t3
			builder.Read(gBufferPassData->GetData().depth, 4); // t4
			builder.Read(shadowTestPassData->GetData().shadowTest, 5); // t5
			builder.Read(pCubeMap, 6);
			builder.Read(pPreFilter, 7);
			builder.Read(pBRDFLut, 8);

			data.lighting = builder.WriteRT(pLit, 0); 
			data.lightingSpec = builder.WriteRT(pLitSpec, 1); 
			data.lightingCopy = builder.WriteRT(pLitCopy, 2); 
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DeferredLightingData& data) {
		});

	struct SubsurfaceData
	{
		NXRGResource* buf;
		NXRGResource* depth;
	};
	auto sssPassData = m_pRenderGraph->AddPass<SubsurfaceData>("Subsurface", new NXSubSurfaceRenderer(),
		[&](NXRGBuilder& builder, SubsurfaceData& data) {
			builder.Read(litPassData->GetData().lighting, 0);
			builder.Read(litPassData->GetData().lightingSpec, 1);
			builder.Read(litPassData->GetData().lightingCopy, 2);
			builder.Read(gBufferPassData->GetData().rt1, 3);
			builder.Read(gBufferPassData->GetData().depth, 4);

			data.buf = builder.WriteRT(litPassData->GetData().lighting, 0, true);
			data.depth = builder.WriteDS(gBufferPassData->GetData().depth, true);

			auto pNoise64 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64);
			auto renderer = static_cast<NXSubSurfaceRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->SetInputTex(pNoise64, 5);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, SubsurfaceData& data) {
		});

	struct SkyLightingData
	{
		NXRGResource* buf;
		NXRGResource* depth;
	};
	auto skyPassData = m_pRenderGraph->AddPass<SkyLightingData>("SkyLighting", new NXSkyRenderer(m_scene),
		[&](NXRGBuilder& builder, SkyLightingData& data) {
			data.buf = builder.WriteRT(sssPassData->GetData().buf, 0, true);
			data.depth = builder.WriteDS(gBufferPassData->GetData().depth, true);

			auto pCubeMap = m_scene->GetCubeMap()->GetCubeMap();
			auto renderer = static_cast<NXSkyRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->SetInputTex(pCubeMap, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, SkyLightingData& data) {
		});

	struct PostProcessingData
	{
		NXRGResource* out;
	};

	auto pPostProcess = m_pRenderGraph->CreateResource("PostProcessing RT", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });

	auto postProcessPassData = m_pRenderGraph->AddPass<PostProcessingData>("PostProcessing", new NXColorMappingRenderer(),
		[&](NXRGBuilder& builder, PostProcessingData& data) {
			builder.Read(skyPassData->GetData().buf, 0);
			data.out = builder.WriteRT(pPostProcess, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, PostProcessingData& data) {
		});

	struct DebugLayerData
	{
		NXRGResource* out;
	};


	auto pDebugLayer = m_pRenderGraph->CreateResource("Debug Layer RT", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });

	auto debugLayerPassData = m_pRenderGraph->AddPass<DebugLayerData>("DebugLayer", new NXDebugLayerRenderer(),
		[&](NXRGBuilder& builder, DebugLayerData& data) {
			auto renderer = static_cast<NXDebugLayerRenderer*>(builder.GetPassNode()->GetRenderPass());

			builder.Read(postProcessPassData->GetData().out, 0);
			builder.Read(pCSMDepth, 1);
			data.out = builder.WriteRT(pDebugLayer, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DebugLayerData& data) {
			auto pDebugLayer = static_cast<NXDebugLayerRenderer*>(m_pRenderGraph->GetRenderPass("DebugLayer"));
			pDebugLayer->SetEnableDebugLayer(m_bEnableDebugLayer);
		});

	struct GizmosData
	{
		NXRGResource* out;
	};
	auto gizmosPassData = m_pRenderGraph->AddPass<GizmosData>("Gizmos", new NXEditorObjectRenderer(m_scene),
		[&](NXRGBuilder& builder, GizmosData& data) {
			//NXRGResource* pOut = m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out;
			NXRGResource* pOut = debugLayerPassData->GetData().out;
			data.out = builder.WriteRT(pOut, 0, true);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, GizmosData& data) {
		});

	m_pRenderGraph->SetPresent(gizmosPassData->GetData().out);
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
		pCubeMap->Update(nullptr);
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

	ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
	pCommandList->SetDescriptorHeaps(1, ppHeaps);

	// 执行RenderGraph!
	m_pRenderGraph->Execute(pCommandList.Get());

	NX12Util::EndEvent(pCommandList.Get());

	pCommandList->Close();

	// 确保BRDF 2D LUT 异步加载完成
	WaitForBRDF2DLUTFinish();

	ID3D12CommandList* pCmdLists[] = { pCommandList.Get() };
	NXGlobalDX::GlobalCmdQueue()->ExecuteCommandLists(1, pCmdLists);

	// 更新PSOManager状态
	NXPSOManager::GetInstance()->FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pRenderGraph->GetPresent(), swapChainBuffer);
}

void Renderer::Release()
{
	// TODO: m_renderGraph release.

	SafeRelease(m_pGUI);
	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);

	NXAllocatorManager::GetInstance()->Release();
	NXSubMeshGeometryEditor::GetInstance()->Release();
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
