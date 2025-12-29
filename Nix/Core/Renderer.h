#pragma once
#include "NXGlobalDefinitions.h"
#include "NXBRDFlut.h"
#include "DirectResources.h"
#include "NXTerrainLODStreamer.h"

#include "NXDepthPrepass.h"
#include "NXForwardRenderer.h"
#include "NXDepthPeelingRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"
#include "NXRenderGraph.h"

struct CBufferShadowMapObject
{
	Matrix view;
	Matrix projection;
};

struct NXEventArgKey;
class NXPassMaterial;
class Renderer
{
public:
	Renderer(const Vector2& rtSize);

	void Init();
	void OnResize(const Vector2& rtSize);

	// 资源重加载（如果上一帧修改了资源）
	void ResourcesReloading(DirectResources* pDXRes);

	void Update();

	void UpdateTime();

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

	Ntr<NXReadbackData>& GetVTReadbackData() { return m_vtReadbackData; }

	// TerrainLODStreamer
	NXTerrainLODStreamer* GetTerrainLODStreamer() { return m_pTerrainLODStreamer; }

private:
	void InitEvents();
	void InitGlobalResources();
	void GenerateRenderGraph();
	void InitGUI();

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

	Ntr<NXReadbackData> m_vtReadbackData;
	Int2 m_vtReadbackDataSize;

	NXTerrainLODStreamer* m_pTerrainLODStreamer;
};