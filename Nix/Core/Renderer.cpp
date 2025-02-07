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

	struct GBufferData
	{
		NXRGResource* depth;
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
	} ;
	auto gBufferPassData = m_pRenderGraph->AddPass<GBufferData>("GBufferPass", new NXGBufferRenderer(m_scene),
		[&](NXRGBuilder& pBuilder, GBufferData& data) {
			NXGBufferRenderer* p = (NXGBufferRenderer*)pBuilder.GetPassNode()->GetRenderPass();
			p->SetCamera(m_scene->GetMainCamera());

			data.rt0 = pBuilder.Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
			data.rt1 = pBuilder.Create({ .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
			data.rt2 = pBuilder.Create({ .format = DXGI_FORMAT_R10G10B10A2_UNORM, .handleFlags = RG_RenderTarget });
			data.rt3 = pBuilder.Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
			data.depth = pBuilder.Create({ .format = DXGI_FORMAT_R24G8_TYPELESS, .handleFlags = RG_DepthStencil });
			data.rt0 = pBuilder.Write(data.rt0);
			data.rt1 = pBuilder.Write(data.rt1);
			data.rt2 = pBuilder.Write(data.rt2);
			data.rt3 = pBuilder.Write(data.rt3);
			data.depth = pBuilder.Write(data.depth);
		}, 
		[=](ID3D12GraphicsCommandList* pCmdList, GBufferData& data) {
			m_pRenderGraph->ClearRT(pCmdList, data.depth);
			m_pRenderGraph->ClearRT(pCmdList, data.rt0);
			m_pRenderGraph->ClearRT(pCmdList, data.rt1);
			m_pRenderGraph->ClearRT(pCmdList, data.rt2);
			m_pRenderGraph->ClearRT(pCmdList, data.rt3);
			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, m_viewRTSize); // 第一个pass设置ViewPort
		});

	struct DepthCopyData
	{
		NXRGResource* depthCopy;
	};
	auto depthCopyPassData = 
	m_pRenderGraph->AddPass<DepthCopyData>("DepthCopy", new NXDepthRenderer(),
		[&](NXRGBuilder& builder, DepthCopyData& data) {
			data.depthCopy = builder.Create({ .format = DXGI_FORMAT_R32_FLOAT, .handleFlags = RG_RenderTarget });
			builder.Read(gBufferPassData->GetData().depth);
			data.depthCopy = builder.Write(data.depthCopy);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DepthCopyData& data) {
		});

	struct ShadowMapData
	{
		// TODO: shadowMap 纹理目前直接写死在NXShadowMapRenderer，改成可配置的
		//NXRGResource* shadowMap;
	} ;
	auto shadowMapPassData = m_pRenderGraph->AddPass<ShadowMapData>("ShadowMap", new NXShadowMapRenderer(m_scene),
		[&](NXRGBuilder& builder, ShadowMapData& data) {
		},
		[&](ID3D12GraphicsCommandList* pCmdList, ShadowMapData& data) {
			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, Vector2(2048, 2048)); // CSM ShadowMap 的 ViewPort
		});

	struct ShadowTestData
	{
		NXRGResource* shadowTest;
	};
	auto shadowTestPassData = m_pRenderGraph->AddPass<ShadowTestData>("ShadowTest", new NXShadowTestRenderer(),
		[&](NXRGBuilder& builder, ShadowTestData& data) {
			data.shadowTest = builder.Create({ .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
			builder.Read(gBufferPassData->GetData().depth); // register t0
			data.shadowTest = builder.Write(data.shadowTest);

			// 从 ShadowMap Pass 获取 CSM 深度纹理，并推入当前 Pass 的输入
			auto pShadowMapPass = static_cast<NXShadowMapRenderer*>(m_pRenderGraph->GetRenderPass("ShadowMap"));
			auto pCSMDepth = pShadowMapPass->GetShadowMapDepthTex();
			builder.GetPassNode()->GetRenderPass()->PushInputTex(pCSMDepth); // register t1
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
	auto litPassData = m_pRenderGraph->AddPass<DeferredLightingData>("DeferredLighting", new NXDeferredRenderer(m_scene),
		[&](NXRGBuilder& builder, DeferredLightingData& data) {
			data.lighting = builder.Create({ .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
			data.lightingSpec = builder.Create({ .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
			data.lightingCopy = builder.Create({ .format = DXGI_FORMAT_R11G11B10_FLOAT,    .handleFlags = RG_RenderTarget });

			builder.Read(gBufferPassData->GetData().rt0); // register t0
			builder.Read(gBufferPassData->GetData().rt1); // t1
			builder.Read(gBufferPassData->GetData().rt2); // t2
			builder.Read(gBufferPassData->GetData().rt3); // t3
			builder.Read(gBufferPassData->GetData().depth); // t4
			builder.Read(shadowTestPassData->GetData().shadowTest); // t5

			data.lighting = builder.Write(data.lighting);
			data.lightingSpec = builder.Write(data.lightingSpec);
			data.lightingCopy = builder.Write(data.lightingCopy);

			auto pCubeMap = m_scene->GetCubeMap()->GetCubeMap();
			auto pPreFilter = m_scene->GetCubeMap()->GetPreFilterMap();
			auto pBRDFLut = m_pBRDFLut->GetTex();
			auto renderer = static_cast<NXDeferredRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->PushInputTex(pCubeMap);    // t6
			renderer->PushInputTex(pPreFilter);    // t7
			renderer->PushInputTex(pBRDFLut);      // t8
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DeferredLightingData& data) {
		});

	struct SubsurfaceData
	{
		NXRGResource* buf;
	};
	auto sssPassData = m_pRenderGraph->AddPass<SubsurfaceData>("Subsurface", new NXSubSurfaceRenderer(),
		[&](NXRGBuilder& builder, SubsurfaceData& data) {
			data.buf = builder.Create({ .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });
			builder.Read(litPassData->GetData().lighting);
			builder.Read(litPassData->GetData().lightingSpec);
			builder.Read(litPassData->GetData().lightingCopy);
			builder.Read(gBufferPassData->GetData().rt1);
			builder.Read(gBufferPassData->GetData().depth);

			data.buf = builder.Write(data.buf);

			auto pNoise64 = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64);
			auto renderer = static_cast<NXSubSurfaceRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->PushInputTex(pNoise64);
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
			data.buf = builder.Write(sssPassData->GetData().buf);
			data.depth = builder.Write(gBufferPassData->GetData().depth);

			auto pCubeMap = m_scene->GetCubeMap()->GetCubeMap();
			auto renderer = static_cast<NXSkyRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->PushInputTex(pCubeMap);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, SkyLightingData& data) {
		});

	struct PostProcessingData
	{
		NXRGResource* out;
	};
	auto postProcessPassData = m_pRenderGraph->AddPass<PostProcessingData>("PostProcessing", new NXColorMappingRenderer(),
		[&](NXRGBuilder& builder, PostProcessingData& data) {
			data.out = builder.Create({ .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });
			builder.Read(skyPassData->GetData().buf);
			data.out = builder.Write(data.out);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, PostProcessingData& data) {
		});

	struct DebugLayerData
	{
		NXRGResource* out;
	};
	auto debugLayerPassData = m_pRenderGraph->AddPass<DebugLayerData>("DebugLayer", new NXDebugLayerRenderer(),
		[&](NXRGBuilder& builder, DebugLayerData& data) {
			auto renderer = static_cast<NXDebugLayerRenderer*>(builder.GetPassNode()->GetRenderPass());
			renderer->OnResize(m_viewRTSize);

			data.out = builder.Create({ .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });
			builder.Read(postProcessPassData->GetData().out);
			data.out = builder.Write(data.out);

			auto pShadowMapPass = static_cast<NXShadowMapRenderer*>(m_pRenderGraph->GetRenderPass("ShadowMap"));
			auto pCSMDepth = pShadowMapPass->GetShadowMapDepthTex();
			m_pRenderGraph->GetRenderPass("ShadowTest")->PushInputTex(pCSMDepth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DebugLayerData& data) {
		});

	struct GizmosData
	{
	};
	auto gizmosPassData = m_pRenderGraph->AddPass<GizmosData>("Gizmos", new NXEditorObjectRenderer(m_scene),
		[&](NXRGBuilder& builder, GizmosData& data) {
		},
		[&](ID3D12GraphicsCommandList* pCmdList, GizmosData& data) {
		});

	m_pRenderGraph->SetPresent(m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out);
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
	if (m_bRenderGUI) m_pGUI->Render(m_pFinalRT, swapChainBuffer);
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
