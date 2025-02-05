#pragma once
#include "NXGlobalDefinitions.h"
#include "NXBRDFlut.h"
#include "DirectResources.h"

#include "NXDepthPrepass.h"
#include "NXGBufferRenderer.h"
#include "NXShadowMapRenderer.h"
#include "NXShadowTestRenderer.h"
#include "NXDeferredRenderer.h"
#include "NXForwardRenderer.h"
#include "NXDepthRenderer.h"
#include "NXDepthPeelingRenderer.h"
#include "NXSubSurfaceRenderer.h"
#include "NXSkyRenderer.h"
#include "NXColorMappingRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"
#include "NXDebugLayerRenderer.h"
#include "NXEditorObjectRenderer.h"
#include "NXRenderGraph.h"

struct NXEventArgKey;
class Renderer
{
public:
	Renderer(const Vector2& rtSize);

	void Init();
	void OnResize(const Vector2& rtSize);

	// 资源重加载（如果上一帧修改了资源）
	void ResourcesReloading();

	void Update();

	void UpdateTime();

	// 当前帧 渲染画面的绘制
	void RenderFrame();

	// 负责处理 GUI 的实际渲染 和 即时更新。见上面 UpdateGUI() 的注释
	void RenderGUI(const NXSwapChainBuffer& swapChainBuffer);

	void Release();

	void ClearAllPSResources();

	NXRenderGraph* GetRenderGraph()				{ return m_pRenderGraph; }
	bool	GetEnableDebugLayer()				{ return m_bEnableDebugLayer; }
	void	SetEnableDebugLayer(bool value)		{ m_bEnableDebugLayer = value; }

private:
	void InitEvents();
	void InitGlobalResources();
	void InitRenderGraph();
	void InitGUI();

	// 2023.11.5 Nix 的 GUI 控制参数目前暂时使用两种方式：即时更新 和 延迟更新
	// 1. 即时更新：GUI 每次修改参数，都会立即更新到对应的资源上（即，传统的 dearImgui 更新参数的方法）
	// 2. 延迟更新：上一帧的 GUI 修改参数后，通过命令队列的形式交给 NXGUICommandManager，等到这一帧 UpdateGUI 再更新
	// UpdateGUI() 负责处理 延迟更新
	// RenderGUI() 负责处理GUI的渲染和 即时更新
	void UpdateGUI();

	// 更新 NXScene 场景
	void UpdateSceneData();

	void DrawDepthPrepass();
	void OnKeyDown(NXEventArgKey eArg);

	void WaitForBRDF2DLUTFinish();

private:
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		m_pCommandAllocator;
	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	m_pCommandList;

	Vector2				m_viewRTSize;
	NXBRDFLut*			m_pBRDFLut;

	NXScene*			m_scene;

	NXRenderGraph*		m_pRenderGraph;
	Ntr<NXTexture2D>	m_pFinalRT;

	bool				m_bEnableDebugLayer;

	NXGUI*				m_pGUI;
	bool				m_bRenderGUI;
};