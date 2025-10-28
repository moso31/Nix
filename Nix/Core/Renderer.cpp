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
#include "NXSubMeshGeometryEditor.h"
#include "NXPSOManager.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"
#include "NXGPUTerrainManager.h"
#include "NXVirtualTextureManager.h"
#include "NXTerrainStreamingBatcher.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_pRenderGraph(nullptr),
	m_viewRTSize(rtSize),
	m_bEnablePostProcessing(true),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f),
	m_pNeedRebuildRenderGraph(true)
{
	m_cbDebugLayerData.LayerParam0 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	m_vtReadbackData = new NXReadbackData("VT Readback CPUdata");
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

	NXGPUTerrainManager::GetInstance()->Init();

	m_scene->Init();
	NXVirtualTextureManager::GetInstance()->Init();
	NXVirtualTextureManager::GetInstance()->BuildSearchList(400);
	NXVirtualTextureManager::GetInstance()->SetCamera(m_scene->GetMainCamera());

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->Init();

	m_pRenderGraph = new NXRenderGraph();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	if (m_pRenderGraph)
		m_pRenderGraph->SetViewResolution(m_viewRTSize);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
	m_pGUI->SetVTReadbackData(m_vtReadbackData);
}

void Renderer::GenerateRenderGraph()
{
	m_pRenderGraph->Destroy();
	m_pRenderGraph->SetViewResolution(m_viewRTSize);


	NXRGResource* pTerrainBufferA = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainBufferA());
	NXRGResource* pTerrainBufferB = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainBufferB());
	NXRGResource* pTerrainBufferFinal = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainFinalBuffer());
	NXRGResource* pTerrainIndiArgs = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainIndirectArgs());
	NXRGResource* pTerrainPatcher = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainPatcherBuffer());
	NXRGResource* pTerrainDrawIndexArgs = m_pRenderGraph->ImportBuffer(NXGPUTerrainManager::GetInstance()->GetTerrainDrawIndexArgs());

	struct FillTestData
	{
		NXComputePass* pFillPass;
	};

	for (int i = 0; i < 6; i++)
	{
		NXRGResource* pInputBuf = i % 2 ? pTerrainBufferB : pTerrainBufferA;
		NXRGResource* pOutputBuf = i % 2 ? pTerrainBufferA : pTerrainBufferB;

		std::string strBufName = "Terrain Fill " + std::to_string(i);
		m_pRenderGraph->AddComputePass<FillTestData>(strBufName, new NXFillTestRenderer(),
			[=](NXRGBuilder& builder, FillTestData& data) {
				data.pFillPass = (NXComputePass*)builder.GetPassNode()->GetRenderPass();
				builder.SetSubmitGroup(0);

				builder.WriteUAV(pInputBuf, 0, true); // u0
				builder.WriteUAV(pOutputBuf, 1, true); // u1
				builder.WriteUAV(pTerrainBufferFinal, 2, true); // u2
				builder.SetIndirectArgs(pTerrainIndiArgs);

				builder.SetRootParamLayout(1, 0, 3);
				builder.ReadConstantBuffer(0, 0, &NXGPUTerrainManager::GetInstance()->GetCBTerrainParams(i));

				builder.SetEntryNameCS(L"CS_Pass");
			},
			[=](ID3D12GraphicsCommandList* pCmdList, FillTestData& data) {
				if (i == 0)
				{
					std::vector<NXGPUTerrainBlockData> initData; // NXGPUTerrainBlockData = Int2
					int step = 4;
					for (int x = -step; x < step; x++)
					{
						for (int y = -step; y < step; y++)
						{
							initData.push_back({ x, y });
						}
					}

					pInputBuf->GetBuffer()->SetCurrent(initData.data(), initData.size());
					pTerrainBufferFinal->GetBuffer()->SetCurrent(nullptr, 0);
				}

				NXGPUTerrainManager::GetInstance()->UpdateLodParams(i);
				data.pFillPass->SetBufferAsIndirectArg(pInputBuf);
			});
	}

	struct GPUTerrainPatcherData 
	{
		NXComputePass* pPatcherPass;
	};

	auto pTerrain_MinMaxZMap2DArray = m_pRenderGraph->ImportTexture(
		NXGPUTerrainManager::GetInstance()->GetTerrainMinMaxZMap2DArray()
	);

	m_pRenderGraph->AddComputePass<GPUTerrainPatcherData>("GPU Terrain Patcher Clear", new NXGPUTerrainPatcherRenderer(),
		[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
			builder.SetSubmitGroup(0);
			builder.SetRootParamLayout(0, 0, 3);
			builder.WriteUAV(pTerrainPatcher, 0, true);
			builder.WriteUAVCounter(pTerrainPatcher, 2); // uav counter 的值也需要清空
			builder.WriteUAV(pTerrainDrawIndexArgs, 1, true);
			builder.SetComputeThreadGroup(1, 1, 1);
			builder.SetEntryNameCS(L"CS_Clear");
		},
		[=](ID3D12GraphicsCommandList* pCmdList, GPUTerrainPatcherData& data) {
		});

	m_pRenderGraph->AddComputePass<GPUTerrainPatcherData>("GPU Terrain Patcher", new NXGPUTerrainPatcherRenderer(),
		[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
			data.pPatcherPass = (NXComputePass*)builder.GetPassNode()->GetRenderPass();
			builder.SetSubmitGroup(0);
			builder.SetRootParamLayout(2, 2, 3);
			builder.ReadConstantBuffer(0, 1, &g_cbCamera);
			builder.ReadConstantBuffer(1, 2, &NXGPUTerrainManager::GetInstance()->GetTerrainSupportParam());
			builder.Read(pTerrain_MinMaxZMap2DArray, 0);
			builder.Read(pTerrainBufferFinal, 1);
			builder.WriteUAV(pTerrainPatcher, 0, true);
			builder.WriteUAV(pTerrainDrawIndexArgs, 1, true);
			builder.SetIndirectArgs(pTerrainIndiArgs);
			builder.SetEntryNameCS(L"CS_Patch");
		},
		[=](ID3D12GraphicsCommandList* pCmdList, GPUTerrainPatcherData& data) {
			data.pPatcherPass->SetBufferAsIndirectArg(pTerrainBufferFinal);
		});

	struct GBufferData
	{
		NXRGResource* depth;
		NXRGResource* rt0;
		NXRGResource* rt1;
		NXRGResource* rt2;
		NXRGResource* rt3;
	};

	NXRGResource* pGBuffer0 = m_pRenderGraph->CreateResource("GBuffer RT0", { .format = DXGI_FORMAT_R32_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer1 = m_pRenderGraph->CreateResource("GBuffer RT1", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer2 = m_pRenderGraph->CreateResource("GBuffer RT2", { .format = DXGI_FORMAT_R10G10B10A2_UNORM, .handleFlags = RG_RenderTarget });
	NXRGResource* pGBuffer3 = m_pRenderGraph->CreateResource("GBuffer RT3", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });
	NXRGResource* pDepthZ = m_pRenderGraph->CreateResource("DepthZ", { .format = DXGI_FORMAT_R24G8_TYPELESS, .handleFlags = RG_DepthStencil });

	auto gBufferPassData = m_pRenderGraph->AddPass<GBufferData>("GBufferPass", new NXGBufferRenderer(m_scene),
		[&](NXRGBuilder& pBuilder, GBufferData& data) {
			NXGBufferRenderer* p = (NXGBufferRenderer*)pBuilder.GetPassNode()->GetRenderPass();
			p->SetCamera(m_scene->GetMainCamera()); // todo：别把绑相机写在这里吧？
			pBuilder.SetSubmitGroup(1);

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

	NXRGResource* pVTReadback = m_pRenderGraph->CreateResource("VT Readback Buffer", { .isViewRT = true, .RTScale = 0.125f, .type = NXResourceType::Buffer, .format = DXGI_FORMAT_R32_FLOAT });
	struct VTReadback
	{
		NXVTReadbackRenderer* pPass;
	};
	m_pRenderGraph->AddComputePass<VTReadback>("VTReadbackPass", new NXVTReadbackRenderer(),
		[&](NXRGBuilder& builder, VTReadback& data) {
			data.pPass = static_cast<NXVTReadbackRenderer*>(builder.GetPassNode()->GetRenderPass());
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(1, 1, 1);
			builder.ReadConstantBuffer(0, 0, &NXVirtualTextureManager::GetInstance()->GetCBufferVTReadback());
			builder.Read(pGBuffer0, 0);
			builder.WriteUAV(pVTReadback, 0);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, VTReadback& data) {
			auto& pRT = pGBuffer0->GetTexture();
			Int2 rtSize(pRT->GetWidth(), pRT->GetHeight());
			Int2 threadGroupSize((rtSize + 7) / 8);
			data.pPass->SetThreadGroups(threadGroupSize.x, threadGroupSize.y);

			// 记录VTReadback的size 
			m_vtReadbackDataSize = threadGroupSize;
			m_pGUI->SetVTReadbackDataSize(threadGroupSize);
		});

	// TODO: 简略变量，new NXReadbackBufferPass有必要吗？
	struct VTReadbackData
	{
	};
	m_pRenderGraph->AddReadbackBufferPass<VTReadbackData>("", new NXReadbackBufferPass(),
		[&](NXRGBuilder& builder, VTReadbackData& data) {
			builder.SetSubmitGroup(2);
			builder.Read(pVTReadback, 0); // TODO：readback类型Pass 的 slot0 没有意义，这种情况下就别传了
			builder.WriteReadbackData(m_vtReadbackData);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, VTReadbackData& data) {
		});

	struct DepthCopyData
	{
		NXRGResource* depthCopy;
	};

	NXRGResource* pDepthCopy = m_pRenderGraph->CreateResource("DepthCopy", { .format = DXGI_FORMAT_R32_FLOAT, .handleFlags = RG_RenderTarget });

	auto depthCopyPassData = 
	m_pRenderGraph->AddPass<DepthCopyData>("DepthCopy", new NXDepthRenderer(),
		[&](NXRGBuilder& builder, DepthCopyData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(0, 1, 0);
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

	auto pCSMDepth = m_pRenderGraph->ImportTexture(pShadowMapDepth, RG_DepthStencil);
	auto shadowMapPassData = m_pRenderGraph->AddPass<ShadowMapData>("ShadowMap", new NXShadowMapRenderer(m_scene),
		[&](NXRGBuilder& builder, ShadowMapData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(2, 0, 0); 
			builder.ReadConstantBuffer(0, 0, &g_cbObject); 
			builder.ReadConstantBuffer(1, 2, &g_cbShadowTest);
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
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(3, 2, 0);
			builder.ReadConstantBuffer(0, 0, &g_cbObject); 
			builder.ReadConstantBuffer(1, 1, &g_cbCamera); 
			builder.ReadConstantBuffer(2, 2, &g_cbShadowTest); 
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
	auto pCubeMap = m_pRenderGraph->ImportTexture(m_scene->GetCubeMap()->GetCubeMap());
	auto pPreFilter = m_pRenderGraph->ImportTexture(m_scene->GetCubeMap()->GetPreFilterMap());
	auto pBRDFLut = m_pRenderGraph->ImportTexture(m_pBRDFLut->GetTex());

	auto litPassData = m_pRenderGraph->AddPass<DeferredLightingData>("DeferredLighting", new NXDeferredRenderer(m_scene),
		[&](NXRGBuilder& builder, DeferredLightingData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(5, 9, 0);
			builder.ReadConstantBuffer(0, 0, &g_cbObject); // b0
			builder.ReadConstantBuffer(1, 1, &g_cbCamera); // b1
			builder.ReadConstantBuffer(2, 2, &m_scene->GetConstantBufferLights()); // b2
			builder.ReadConstantBuffer(3, 3, &m_scene->GetCubeMap()->GetCBDataParams()); // b3
			builder.ReadConstantBuffer(4, 4, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile()); // b4

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
	auto pNoise64 = m_pRenderGraph->ImportTexture(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64));
	auto sssPassData = m_pRenderGraph->AddPass<SubsurfaceData>("Subsurface", new NXSubSurfaceRenderer(),
		[&](NXRGBuilder& builder, SubsurfaceData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(2, 6, 0);
			builder.ReadConstantBuffer(0, 1, &g_cbCamera);
			builder.ReadConstantBuffer(1, 3, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

			builder.Read(litPassData->GetData().lighting, 0);
			builder.Read(litPassData->GetData().lightingSpec, 1);
			builder.Read(litPassData->GetData().lightingCopy, 2);
			builder.Read(gBufferPassData->GetData().rt1, 3);
			builder.Read(gBufferPassData->GetData().depth, 4);
			builder.Read(pNoise64, 5);

			data.buf = builder.WriteRT(litPassData->GetData().lighting, 0, true);
			data.depth = builder.WriteDS(gBufferPassData->GetData().depth, true);
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
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(2, 1, 0);
			builder.ReadConstantBuffer(0, 0, &m_scene->GetCubeMap()->GetCBObjectParams());
			builder.ReadConstantBuffer(1, 1, &m_scene->GetCubeMap()->GetCBDataParams());
			builder.Read(pCubeMap, 0);

			data.buf = builder.WriteRT(sssPassData->GetData().buf, 0, true);
			data.depth = builder.WriteDS(gBufferPassData->GetData().depth, true);
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
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(1, 1, 0);
			builder.ReadConstantBuffer(0, 2, &m_cbColorMapping);
			builder.Read(skyPassData->GetData().buf, 0);
			data.out = builder.WriteRT(pPostProcess, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, PostProcessingData& data) {
			m_cbColorMappingData.param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
			m_cbColorMapping.Update(m_cbColorMappingData);
		});

	struct DebugLayerData
	{
		NXRGResource* out;
	};

	auto pDebugLayer = m_pRenderGraph->CreateResource("Debug Layer RT", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });

	auto debugLayerPassData = m_pRenderGraph->AddPass<DebugLayerData>("DebugLayer", new NXDebugLayerRenderer(),
		[&](NXRGBuilder& builder, DebugLayerData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(2, 2, 0);
			builder.ReadConstantBuffer(0, 1, &g_cbCamera);
			builder.ReadConstantBuffer(1, 2, &m_cbDebugLayer);
			builder.Read(postProcessPassData->GetData().out, 0);
			builder.Read(pCSMDepth, 1);
			data.out = builder.WriteRT(pDebugLayer, 0);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DebugLayerData& data) {
			m_cbDebugLayerData.LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
			m_cbDebugLayerData.LayerParam0.y = m_fShadowMapZoomScale;
			m_cbDebugLayer.Update(m_cbDebugLayerData);
		});

	struct GizmosData
	{
		NXRGResource* out;
	};
	auto gizmosPassData = m_pRenderGraph->AddPass<GizmosData>("Gizmos", new NXEditorObjectRenderer(m_scene),
		[&](NXRGBuilder& builder, GizmosData& data) {
			builder.SetSubmitGroup(2);
			builder.SetRootParamLayout(3, 0, 0);
			builder.ReadConstantBuffer(1, 1, &g_cbCamera);
			NXRGResource* pOut = m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out;
			//NXRGResource* pOut = debugLayerPassData->GetData().out;
			data.out = builder.WriteRT(pOut, 0, true);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, GizmosData& data) {
		});

	m_pRenderGraph->SetPresent(gizmosPassData->GetData().out);
}
 
void Renderer::NotifyRebuildRenderGraph()
{
	m_pNeedRebuildRenderGraph = true;
}

void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::InitGlobalResources()
{
	NXGlobalInputLayout::Init();
}

void Renderer::ResourcesReloading(DirectResources* pDXRes)
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();

	if (m_pNeedRebuildRenderGraph)
	{
		pDXRes->Flush();
		GenerateRenderGraph();
		m_pNeedRebuildRenderGraph = false;
	}

	// 每帧都Compile
	m_pRenderGraph->Compile();
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

	auto* pCamera = m_scene->GetMainCamera();
	NXGPUTerrainManager::GetInstance()->UpdateCameraParams(pCamera);
	NXVirtualTextureManager::GetInstance()->Update();
	NXVirtualTextureManager::GetInstance()->UpdateCBData(pCamera->GetRTSize());

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
	// 确保BRDF 2D LUT 异步加载完成
	m_pBRDFLut->WaitTexLoadFinish();

	// 执行RenderGraph!
	m_pRenderGraph->Execute();

	// 更新PSOManager状态
	NXPSOManager::GetInstance()->FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI) m_pGUI->Render(m_pRenderGraph->GetPresent(), swapChainBuffer);
}

void Renderer::Release()
{
	m_pRenderGraph->Destroy();

	SafeRelease(m_pGUI);
	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);

	NXVirtualTextureManager::GetInstance()->Release();
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
