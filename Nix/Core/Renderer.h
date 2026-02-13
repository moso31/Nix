#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "NXConstantBuffer.h"
#include "Ntr.h"  
#include "NXRGUtil.h"

// ===== 前置声明 =====
// 场景/渲染相关
class NXScene;
class NXRenderGraph;
class NXGUI;
class NXBRDFLut;
class NXTerrainLODStreamer;
class NXVirtualTexture;
class NXPassMaterial;
class NXPBRDistantLight;
class NXRenderableObject;

// 资源相关
class NXTexture2D;
class NXTexture2DArray;
class DirectResources;

// RenderGraph Pass 相关
template<typename T> class NXRGPassNode;
struct TerrainPatcherPassData;
struct GBufferPassData;
struct ShadowMapPassData;
struct ShadowTestPassData;
struct DeferredLightingPassData;
struct SubsurfacePassData;
struct SkyLightingPassData;
struct PostProcessingPassData;
struct DebugLayerPassData;
struct GizmosPassData;

// 事件
struct NXEventArgKey;
struct NXSwapChainBuffer;

struct CBufferShadowMapObject
{
	Matrix view;
	Matrix projection;
};

class Renderer
{
public:
	Renderer(const Vector2& rtSize);

	void Init();
	void OnResize(const Vector2& rtSize);

	// 资源重加载（如果上一帧修改了资源）
	void ResourcesReloading(DirectResources* pDXRes);

	void Update();

	void UpdateGlobalCBuffer();

	// 当前帧 渲染画面的绘制
	void RenderFrame();

	// 负责处理 GUI 的实际渲染 和 即时更新。见上面 UpdateGUI() 的注释
	void RenderGUI(const NXSwapChainBuffer& swapChainBuffer);

	// 帧结束时
	void FrameEnd();

	void Release();

	NXRenderGraph* GetRenderGraph() { return m_pRenderGraph; }
	// shadow map
	float GetShadowMapShadowExponent() const { return m_shadowMap_shadowExponent; }
	void SetShadowMapShadowExponent(float value) { m_shadowMap_shadowExponent = value; }
	// debug layer
	bool GetEnableDebugLayer() const { return m_bEnableDebugLayer; }
	void SetEnableDebugLayer(bool value) { m_bEnableDebugLayer = value; }
	bool GetEnableShadowMapDebugLayer() { return m_bEnableShadowMapDebugLayer; }
	void SetEnableShadowMapDebugLayer(bool value) { m_bEnableShadowMapDebugLayer = value; }
	float GetShadowMapDebugLayerZoomScale() { return m_fShadowMapZoomScale; }
	void SetShadowMapDebugLayerZoomScale(float value) { m_fShadowMapZoomScale = value; }
	// post processing
	bool GetEnablePostProcessing() const { return m_bEnablePostProcessing; }
	void SetEnablePostProcessing(bool value) { m_bEnablePostProcessing = value; }
	// FinalQuad
	Ntr<NXTexture2D> GetFinalRT() { return m_pFinalRT; }

	// TerrainLODStreamer
	NXTerrainLODStreamer* GetTerrainLODStreamer() { return m_pTerrainLODStreamer; }

	// VirtualTexture
	NXVirtualTexture* GetVirtualTexture() { return m_pVirtualTexture; }

private:
	void InitEvents();
	void InitGlobalResources();
	void GenerateRenderGraph();
	void InitGUI();

	// ===== RenderGraph Pass 构建辅助函数 =====
	// 将大型的 GenerateRenderGraph() 拆分为多个小函数，改善 IntelliSense 性能
	void BuildTerrainStreamingPasses(NXRGHandle hSector2VirtImg, NXRGHandle pSector2NodeIDTex, NXRGHandle hHeightMapAtlas, NXRGHandle hSplatMapAtlas, NXRGHandle hNormalMapAtlas, NXRGHandle hAlbedoMapAtlas);
	NXRGPassNode<TerrainPatcherPassData>* BuildTerrainCullingPasses(NXRGHandle pSector2NodeIDTex, NXRGHandle& hPatcherBuffer, NXRGHandle& hPatcherDrawIndexArgs);
	NXRGPassNode<GBufferPassData>* BuildGBufferPasses(NXRGPassNode<TerrainPatcherPassData>* passPatcher, NXRGHandle hGBuffer0, NXRGHandle hGBuffer1, NXRGHandle hGBuffer2, NXRGHandle hGBuffer3, NXRGHandle hDepthZ, NXRGHandle hVTPageIDTexture, NXRGHandle hVTSector2VirtImg, NXRGHandle hVTIndirectTexture, NXRGHandle hVTPhysicalPageAlbedo, NXRGHandle hVTPhysicalPageNormal);
	NXRGPassNode<ShadowMapPassData>* BuildShadowMapPass(NXRGPassNode<TerrainPatcherPassData>* passPatcher, NXRGHandle pCSMDepth);
	NXRGPassNode<ShadowTestPassData>* BuildShadowTestPass(NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGPassNode<ShadowMapPassData>* shadowMapPassData);
	NXRGPassNode<DeferredLightingPassData>* BuildDeferredLightingPass(NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGPassNode<ShadowTestPassData>* shadowTestPassData, NXRGHandle pCubeMap, NXRGHandle pPreFilter, NXRGHandle pBRDFLut);
	NXRGPassNode<SubsurfacePassData>* BuildSubsurfacePass(NXRGPassNode<DeferredLightingPassData>* litPassData, NXRGPassNode<GBufferPassData>* gBufferPassData);
	NXRGPassNode<SkyLightingPassData>* BuildSkyLightingPass(NXRGPassNode<SubsurfacePassData>* sssPassData, NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGHandle pCubeMap);
	NXRGPassNode<PostProcessingPassData>* BuildPostProcessingPass(NXRGPassNode<SkyLightingPassData>* skyPassData);
	NXRGPassNode<DebugLayerPassData>* BuildDebugLayerPass(NXRGPassNode<PostProcessingPassData>* postProcessPassData, NXRGPassNode<ShadowMapPassData>* shadowMapPassData);
	NXRGPassNode<GizmosPassData>* BuildGizmosPass(NXRGPassNode<DebugLayerPassData>* debugLayerPassData, NXRGPassNode<PostProcessingPassData>* postProcessPassData);
	void BuildFinalQuadPass(NXRGPassNode<GizmosPassData>* gizmosPassData);

	// 2023.11.5 Nix 的 GUI 控制参数目前暂时使用两种方式：即时更新 和 延迟更新
	// 1. 即时更新：GUI 每次修改参数，都会立即更新到对应的资源上（即，传统的 dearImgui 更新参数的方法）
	// 2. 延迟更新：上一帧的 GUI 修改参数后，通过命令队列的形式交给 NXGUICommandManager，等到这一帧 UpdateGUI 再更新
	// UpdateGUI() 负责处理 延迟更新
	// RenderGUI() 负责处理GUI的渲染和 即时更新
	void UpdateGUI();

	// 更新 NXScene 场景
	void UpdateSceneData();

	void OnKeyDown(NXEventArgKey eArg);

	// shadow map
	void RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, class NXPBRDistantLight* pDirLight);
	void RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, class NXRenderableObject* pRenderableObject);

private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_pCommandSig;

	Ntr<NXTexture2D>	m_pFinalRT;
	Vector2				m_viewRTSize;
	NXBRDFLut*			m_pBRDFLut;

	NXScene*			m_scene;

	NXRenderGraph*		m_pRenderGraph;

	NXGUI*				m_pGUI;
	bool				m_bRenderGUI;

	// shadow map
	float m_shadowMap_shadowExponent = 2.0f;
	uint32_t m_shadowMapRTSize = 2048;
	uint32_t m_cascadeCount = 4;
	Ntr<NXTexture2DArray> m_pTexCSMDepth;
	CBufferShadowMapObject m_cbDataCSMViewProj[8];
	NXConstantBuffer<CBufferShadowMapObject> m_cbCSMViewProj[8];

	// post processing
	bool m_bEnablePostProcessing;
	Vector4 m_cbColorMappingData;
	NXConstantBuffer<Vector4> m_cbColorMapping;

	// debug layer
	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
	Vector4 m_cbDebugLayerData;
	NXConstantBuffer<Vector4> m_cbDebugLayer;

	NXTerrainLODStreamer* m_pTerrainLODStreamer;
	NXVirtualTexture* m_pVirtualTexture;
};