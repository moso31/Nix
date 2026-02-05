#include "Renderer.h"
#include "RenderGraphPassData.h"

// ===== Renderer.h 使用前置声明的头文件，在此完整包含 =====
#include "NXGlobalDefinitions.h"
#include "NXBRDFlut.h"
#include "DirectResources.h"
#include "NXTerrainLODStreamer.h"
#include "NXVirtualTexture.h"
#include "NXGUI.h"
#include "NXRenderGraph.h"
#include "NXScene.h"

// ===== 其他依赖 =====
#include "NXTimer.h"
#include "NXGlobalBuffers.h"
#include "ShaderComplier.h"
#include "NXEvent.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXTexture.h"
#include "NXBuffer.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXPSOManager.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"
#include "NXTerrainCommandSignature.h"
#include "NXTerrainStreamingBatcher.h"
#include "NXPassMaterial.h"
#include "NXPassMaterialManager.h"
#include "NXPBRLight.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"
#include "NXAllocatorManager.h"
#include "NXCamera.h"
#include "NXGPUProfiler.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_pRenderGraph(nullptr),
	m_viewRTSize(rtSize),
	m_bEnablePostProcessing(true),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
	m_cbDebugLayerData = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
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

	NXTerrainCommandSignature::GetInstance()->Init();

	m_scene->Init();
	m_pTerrainLODStreamer = new NXTerrainLODStreamer();
	m_pTerrainLODStreamer->Init(m_scene);
	m_pVirtualTexture = new NXVirtualTexture(m_scene->GetMainCamera());

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->Init();

	m_pRenderGraph = new NXRenderGraph();

	NXPassMng->InitDefaultRenderer();

	// 初始化 GPU Profiler
	g_pGPUProfiler = new NXGPUProfiler();
	g_pGPUProfiler->Init(NXGlobalDX::GetDevice(), NXGlobalDX::GlobalCmdQueue(), 256);

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	m_pFinalRT = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture("Final RT", DXGI_FORMAT_R11G11B10_FLOAT, (uint32_t)m_viewRTSize.x, (uint32_t)m_viewRTSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
}

void Renderer::GenerateRenderGraph()
{
	// 这里的 RenderGraph 设计还比较初级，后续可能会有较大改动。目前规则：
	// setup：
	// - 准确的Read Write所需资源。Consume视为Read，Append视为Write（如果RW行为都有，就都调用）
	// - indirect args资源提交时，视为Read; 
	// execute:
	// - pCmdList目前直接显式暴露，包括资源状态切换、设置indirectArgs、乃至一些memcpy行为、全手动处理

	// 创建 GBuffer 资源
	NXRGHandle hGBuffer0 = m_pRenderGraph->Create("GBuffer RT0", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer1 = m_pRenderGraph->Create("GBuffer RT1", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer2 = m_pRenderGraph->Create("GBuffer RT2", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R10G10B10A2_UNORM, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer3 = m_pRenderGraph->Create("GBuffer RT3", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hDepthZ = m_pRenderGraph->Create("DepthZ", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::DepthStencil, .tex = { .format = DXGI_FORMAT_R24G8_TYPELESS, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	//NXRGHandle hVTPageIDTexture = m_pRenderGraph->Create("VT PageID Texture", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::UnorderedAccess, .tex = { .format = DXGI_FORMAT_R32_UINT, .width = (uint32_t)(m_viewRTSize.x + 7) / 8, .height = (uint32_t)(m_viewRTSize.y + 7) / 8, .arraySize = 1, .mipLevels = 1 }}); // 1/8 RT resolution
	NXRGHandle hVTPageIDTexture = m_pRenderGraph->Create("VT PageID Texture", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::UnorderedAccess, .tex = { .format = DXGI_FORMAT_R32_UINT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 }}); // Full resolution for debugging

	NXRGHandle hVTSector2VirtImg = m_pRenderGraph->Import(m_pVirtualTexture->GetSector2VirtImg());
	NXRGHandle hVTIndirectTexture = m_pRenderGraph->Import(m_pVirtualTexture->GetIndirectTexture());
	NXRGHandle hVTPhysicalPageAlbedo = m_pRenderGraph->Import(m_pVirtualTexture->GetPhysicalPageAlbedo());
	NXRGHandle hVTPhysicalPageNormal = m_pRenderGraph->Import(m_pVirtualTexture->GetPhysicalPageNormal());

	auto& pStreamingData = m_pTerrainLODStreamer->GetStreamingData();
	NXRGHandle pSector2NodeIDTex = m_pRenderGraph->Import(pStreamingData.GetSector2NodeIDTexture());
	NXRGHandle hHeightMapAtlas = m_pRenderGraph->Import(pStreamingData.GetHeightMapAtlas());
	NXRGHandle hSplatMapAtlas = m_pRenderGraph->Import(pStreamingData.GetSplatMapAtlas());
	NXRGHandle hNormalMapAtlas = m_pRenderGraph->Import(pStreamingData.GetNormalMapAtlas());
	NXRGHandle hAlbedoMapArray = m_pRenderGraph->Import(pStreamingData.GetTerrainAlbedo2DArray());
	NXRGHandle hNormalMapArray = m_pRenderGraph->Import(pStreamingData.GetTerrainNormal2DArray());

	NXRGHandle pCubeMap = m_pRenderGraph->Import(m_scene->GetCubeMap()->GetCubeMap());
	NXRGHandle pPreFilter = m_pRenderGraph->Import(m_scene->GetCubeMap()->GetPreFilterMap());
	NXRGHandle pBRDFLut = m_pRenderGraph->Import(m_pBRDFLut->GetTex());

	NXRGHandle pCSMDepth = m_pRenderGraph->Import(m_pTexCSMDepth);

	NXRGPassNode<TerrainPatcherPassData>* passPatcher = nullptr;
	NXRGHandle hPatcherBuffer, hPatcherDrawIndexArgs;

	{
		NXVTRenderGraphContext ctx;
		ctx.pRG = m_pRenderGraph;
		ctx.pTerrainLODStreamer = m_pTerrainLODStreamer;

		ctx.hSector2VirtImg = hVTSector2VirtImg;
		ctx.hSector2NodeIDTex = pSector2NodeIDTex;
		ctx.hHeightMapAtlas = hHeightMapAtlas;
		ctx.hSplatMapAtlas = hSplatMapAtlas;
		ctx.hNormalMapAtlas = hNormalMapAtlas;
		ctx.hAlbedoMapArray = hAlbedoMapArray;
		ctx.hNormalMapArray = hNormalMapArray;
		ctx.hAlbedoPhysicalPage = hVTPhysicalPageAlbedo;
		ctx.hNormalPhysicalPage = hVTPhysicalPageNormal;
		ctx.hIndirectTexture = hVTIndirectTexture;
		m_pVirtualTexture->RegisterRenderPasses(ctx);
	}

	if (g_debug_temporal_enable_terrain_debug)
	{
		// 地形流式加载相关 Pass
		BuildTerrainStreamingPasses(hVTSector2VirtImg, pSector2NodeIDTex, hHeightMapAtlas, hSplatMapAtlas, hNormalMapAtlas);

		// 地形裁剪相关 Pass
		passPatcher = BuildTerrainCullingPasses(pSector2NodeIDTex, hPatcherBuffer, hPatcherDrawIndexArgs);

		auto* pCamera = m_scene->GetMainCamera();
		m_pVirtualTexture->Update();
		m_pVirtualTexture->UpdateCBData(pCamera->GetRTSize());
	}

	// GBuffer Pass
	auto gBufferPassData = BuildGBufferPasses(passPatcher, hGBuffer0, hGBuffer1, hGBuffer2, hGBuffer3, hDepthZ, hVTPageIDTexture, hVTSector2VirtImg, hVTIndirectTexture, hVTPhysicalPageAlbedo, hVTPhysicalPageNormal);

	// VT Readback Pass
	struct VTReadbackData { NXRGHandle vtReadback; };
	auto vtReadbackDataPassData = m_pRenderGraph->AddPass<VTReadbackData>("DoVTReadback",
		[&](NXRGBuilder& builder, VTReadbackData& data) {
			data.vtReadback = builder.Read(gBufferPassData->GetData().VTPageIDTexture);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, VTReadbackData& data) {
			if (m_pVirtualTexture->GetUpdateState() != NXVTUpdateState::WaitReadback)
				return;

			m_pVirtualTexture->SetUpdateState(NXVTUpdateState::Reading);

			auto pTex = resMap.GetRes(data.vtReadback).As<NXTexture2D>();
			m_pVirtualTexture->SetVTReadbackDataSize(Int2(pTex->GetWidth(), pTex->GetHeight()));

			auto pMat = static_cast<NXReadbackPassMaterial*>(NXPassMng->GetPassMaterial("VTReadbackData"));
			pMat->SetInput(pTex);
			pMat->SetOutput(m_pVirtualTexture->GetVTReadbackData());
			pMat->Render(pCmdList);
			pMat->SetCallback([=]() {
				m_pVirtualTexture->UpdateStateAfterReadback();
				});
		});

	// Shadow Passes
	auto shadowMapPassData = BuildShadowMapPass(passPatcher, pCSMDepth);
	auto shadowTestPassData = BuildShadowTestPass(gBufferPassData, shadowMapPassData);

	// Lighting Passes
	auto litPassData = BuildDeferredLightingPass(gBufferPassData, shadowTestPassData, pCubeMap, pPreFilter, pBRDFLut);
	auto sssPassData = BuildSubsurfacePass(litPassData, gBufferPassData);

	// Sky Pass
	auto skyPassData = BuildSkyLightingPass(sssPassData, gBufferPassData, pCubeMap);

	// Post Processing Passes
	auto postProcessPassData = BuildPostProcessingPass(skyPassData);
	auto debugLayerPassData = BuildDebugLayerPass(postProcessPassData, shadowMapPassData);
	auto gizmosPassData = BuildGizmosPass(debugLayerPassData, postProcessPassData);

	// Final Output
	BuildFinalQuadPass(gizmosPassData);
}

void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::InitGlobalResources()
{
	// GlobalInputLayout
	NXGlobalInputLayout::Init();

	// indirectArgs
	D3D12_INDIRECT_ARGUMENT_DESC indirectArgDesc = {};
	indirectArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	desc.pArgumentDescs = &indirectArgDesc;
	desc.NumArgumentDescs = 1;
	desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
	desc.NodeMask = 0;
	m_pCommandSig = NX12Util::CreateCommandSignature(NXGlobalDX::GetDevice(), desc, nullptr);

	// shadow map
	m_pTexCSMDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture2DArray("CSM DepZ 2DArray", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false);
	m_pTexCSMDepth->SetViews(1, 0, m_cascadeCount, 0);
	for (UINT i = 0; i < m_cascadeCount; i++) m_pTexCSMDepth->SetDSV(i, i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pTexCSMDepth->SetSRV(0, 0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）
	m_pTexCSMDepth->SetSRVPreviewsManual(1);
}

void Renderer::ResourcesReloading(DirectResources* pDXRes)
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();
}

void Renderer::Update()
{
	m_pTerrainLODStreamer->ProcessCompletedStreamingTask();
	m_pTerrainLODStreamer->Update();
	m_pTerrainLODStreamer->UpdateAsyncLoader();

	// 每帧都Compile RenderGraph
	m_pRenderGraph->Clear();

	// 2025.12.29 新增GUI的Update，先实现在这里
	// 注册RenderGraph pass会用到，肯定得写在m_pRenderGraph->Clear()和Compile()之间
	// 预计后续还会扩展，根据情况再调整
	m_pGUI->Update();

	GenerateRenderGraph();

	UpdateGUI();
	UpdateSceneData();

	m_pRenderGraph->Compile();
}

void Renderer::UpdateGUI()
{
	m_pGUI->ExecuteDeferredCommands();
}

void Renderer::UpdateSceneData()
{
	UpdateGlobalCBuffer();

	// 更新场景Scripts。实际上是用Scripts控制指定物体的Transform。
	m_scene->UpdateScripts();

	// 更新Transform
	m_scene->UpdateTransform();
	m_scene->UpdateTransformOfEditorObjects();

	// 更新Camera的常量缓存数据（VP矩阵、眼睛位置）
	m_scene->UpdateCamera();

	auto* pCamera = m_scene->GetMainCamera();
	m_pTerrainLODStreamer->GetStreamingData().UpdateCullingData(pCamera);

	m_scene->UpdateLightData();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update(nullptr);
	}

	//m_pSSAO->Update();
}

void Renderer::UpdateGlobalCBuffer()
{
	g_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();
	g_cbDataObject.globalData.frameIndex = (uint32_t)NXGlobalApp::s_frameIndex.load();
}

void Renderer::RenderFrame()
{
	// 确保BRDF 2D LUT 异步加载完成
	m_pBRDFLut->WaitTexLoadFinish();

	// 执行RenderGraph!
	m_pRenderGraph->Execute();

	// 更新PSOManager状态
	NXPSOManager::GetInstance()->FrameCleanup();

	// 延迟清除用不着的 流式地形Tile纹理
	m_pTerrainLODStreamer->GetStreamingData().FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI && m_pFinalRT.IsValid())
	{
		m_pGUI->Render(m_pFinalRT, swapChainBuffer);
	}
}

void Renderer::FrameEnd()
{
	// 释放不需要的PassMaterial
	NXPassMng->FrameCleanup();
}

void Renderer::Release()
{
	m_pRenderGraph->Clear();
	SafeDelete(m_pRenderGraph);

	// 释放 GPU Profiler
	if (g_pGPUProfiler)
	{
		g_pGPUProfiler->Release();
		SafeDelete(g_pGPUProfiler);
	}

	SafeReleaseCOM(m_pCommandSig);

	SafeRelease(m_pGUI);
	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);

	m_pVirtualTexture->Release();
	SafeDelete(m_pVirtualTexture);
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

void Renderer::RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, NXPBRDistantLight* pDirLight)
{
	// 设置test_transition参数
	g_cbDataShadowTest.test_transition = 1.0f; // 默认值

	Vector3 lightDirection = pDirLight->GetDirection();
	lightDirection = lightDirection.IsZero() ? Vector3(0.0f, 0.0f, 1.0f) : lightDirection;
	lightDirection.Normalize();

	NXCamera* pCamera = m_scene->GetMainCamera();
	Vector3 cameraPosition = pCamera->GetTranslation();
	Vector3 cameraDirection = pCamera->GetForward();

	Matrix mxCamViewProjInv = pCamera->GetViewProjectionInverseMatrix();
	Matrix mxCamProjInv = pCamera->GetProjectionInverseMatrix();

	float zNear = pCamera->GetZNear();
	float shadowDistance = g_cbDataShadowTest.shadowDistance;
	float zLength = shadowDistance - zNear;

	Matrix mxCamProj = pCamera->GetProjectionMatrix();

	UINT cascadeCount = g_cbDataShadowTest.cascadeCount;
	float cascadeExponentScale = 2.5f; // 常用值
	float cascadeTransitionScale = g_cbDataShadowTest.cascadeTransitionScale;
	uint32_t shadowMapRTSize = 2048;

	float expScale = 1.0f;
	float sumInv = 1.0f;
	for (UINT i = 1; i < cascadeCount; i++)
	{
		expScale *= cascadeExponentScale;
		sumInv += expScale;
	}
	sumInv = 1.0f / sumInv;

	expScale = 1.0f;
	float percentage = 0.0f;
	float zLastCascadeTransitionLength = 0.0f;
	for (UINT i = 0; i < cascadeCount; i++)
	{
		// 按等比数列划分 cascade
		float percentageOffset = expScale * sumInv;
		expScale *= cascadeExponentScale;

		float zCascadeNear = zNear + percentage * zLength;
		percentage += percentageOffset;
		float zCascadeFar = zNear + percentage * zLength;
		float zCascadeLength = zCascadeFar - zCascadeNear;

		zCascadeNear -= zLastCascadeTransitionLength;

		// 此数值 用于 cascade 之间的平滑过渡
		zLastCascadeTransitionLength = zCascadeLength * cascadeTransitionScale;

		zCascadeLength += zLastCascadeTransitionLength;

		g_cbDataShadowTest.frustumParams[i] = Vector4(zCascadeFar, zLastCascadeTransitionLength, 0.0f, 0.0f);

		float zCascadeNearProj = (zCascadeNear * mxCamProj._33 + mxCamProj._43) / zCascadeNear;
		float zCascadeFarProj = (zCascadeFar * mxCamProj._33 + mxCamProj._43) / zCascadeFar;

		// 计算各层 cascade 的 Frustum (view space)
		Vector3 viewFrustum[8];
		viewFrustum[0] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[1] = Vector3::Transform(Vector3(-1.0f, 1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[2] = Vector3::Transform(Vector3(1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[3] = Vector3::Transform(Vector3(1.0f, 1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[4] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[5] = Vector3::Transform(Vector3(-1.0f, 1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[6] = Vector3::Transform(Vector3(1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[7] = Vector3::Transform(Vector3(1.0f, 1.0f, zCascadeFarProj), mxCamProjInv);

		// 计算 Frustum 的外接球
		float a2 = (viewFrustum[3] - viewFrustum[0]).LengthSquared();
		float b2 = (viewFrustum[7] - viewFrustum[4]).LengthSquared();
		float delta = zCascadeLength * 0.5f + (a2 - b2) / (8.0f * zCascadeLength);

		// 计算 外接球 的 球心，view space 和 world space 都要。
		// zCascadeDistance: 当前 cascade 中 Near平面中心点 到 frustum 外接球心 的距离
		float zCascadeDistance = zCascadeLength - delta;
		Vector3 sphereCenterVS = Vector3(0.0f, 0.0f, zCascadeNear + zCascadeDistance);
		Vector3 sphereCenterWS = cameraPosition + cameraDirection * sphereCenterVS.z;

		// 计算 外接球 的 半径
		float sphereRadius = sqrtf(zCascadeDistance * zCascadeDistance + (a2 * 0.25f));

		Vector3 shadowMapEye = Vector3(0.0f);
		Vector3 shadowMapAt = -lightDirection;
		Vector3 shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		Matrix mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		float cascadeBound = sphereRadius * 2.0f;
		float worldUnitsPerPixel = cascadeBound / shadowMapRTSize;

		// "LS" = "light space" = shadow camera ortho space.
		Vector3 sphereCenterLS = Vector3::Transform(sphereCenterWS, mxShadowView);
		sphereCenterLS -= Vector3(fmodf(sphereCenterLS.x, worldUnitsPerPixel), fmodf(sphereCenterLS.y, worldUnitsPerPixel), 0.0f);
		sphereCenterWS = Vector3::Transform(sphereCenterLS, mxShadowView.Invert());

		Vector3 sceneCenter = m_scene->GetBoundingSphere().Center;
		float sceneRadius = m_scene->GetBoundingSphere().Radius;
		float backDistance = Vector3::Distance(sceneCenter, sphereCenterWS) + sphereRadius;
		shadowMapEye = sphereCenterWS - lightDirection * backDistance;
		shadowMapAt = sphereCenterWS;
		shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		// 2022.5.15 目前平行光 proj 的矩阵方案，对z的范围取值很保守。可以改进
		Matrix mxShadowProj = XMMatrixOrthographicOffCenterLH(-sphereRadius, sphereRadius, -sphereRadius, sphereRadius, 0.0f, backDistance * 2.0f);

		// 更新当前 cascade 层 的 ShadowMap view proj 绘制矩阵
		m_cbDataCSMViewProj[i].view = mxShadowView.Transpose();
		m_cbDataCSMViewProj[i].projection = mxShadowProj.Transpose();
		m_cbCSMViewProj[i].Update(m_cbDataCSMViewProj[i]);
		g_cbDataShadowTest.view[i] = mxShadowView.Transpose();
		g_cbDataShadowTest.projection[i] = mxShadowProj.Transpose();

		auto pCSMDepthDSV = m_pTexCSMDepth->GetDSV(i);
		pCmdList->ClearDepthStencilView(pCSMDepthDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
		pCmdList->OMSetRenderTargets(0, nullptr, false, &pCSMDepthDSV);
		pCmdList->SetGraphicsRootConstantBufferView(2, m_cbCSMViewProj[i].CurrentGPUAddress());
		
		// 更新当前 cascade 层 的 ShadowMap world 绘制矩阵，并绘制
		for (auto pRenderableObj : m_scene->GetRenderableObjects())
		{
			RenderSingleObject(pCmdList, pRenderableObj);
		}
	}

	// Shadow Test
	g_cbShadowTest.Update(g_cbDataShadowTest);
}

void Renderer::RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, NXRenderableObject* pRenderableObject)
{
	NXPrimitive* pPrimitive = pRenderableObject->IsPrimitive();
	if (pPrimitive)
	{
		pPrimitive->Update(pCmdList);

		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubmesh = pPrimitive->GetSubMesh(i);
			pSubmesh->Render(pCmdList);
		}
	}

	for (auto pChildObject : pRenderableObject->GetChilds())
	{
		NXRenderableObject* pChildRenderableObject = pChildObject->IsRenderableObject();
		if (pChildRenderableObject)
			RenderSingleObject(pCmdList, pChildRenderableObject);
	}
}
