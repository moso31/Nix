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
#include "NXFillTestRenderer.h"
#include "NXVirtualTextureRenderer.h"
#include "NXRenderGraph.h"

struct CBufferDebugLayer
{
	Vector4 LayerParam0; // x: EnableShadowMap, y: ZoomScale
};

struct CBufferFillTest
{
	Vector4 Param0; // xyz: camPos, w: currLodLevel;
};

struct NXEventArgKey;
class Renderer
{
public:
	Renderer(const Vector2& rtSize);

	void Init();
	void OnResize(const Vector2& rtSize);

	// ��Դ�ؼ��أ������һ֡�޸�����Դ��
	void ResourcesReloading(DirectResources* pDXRes);

	void Update();

	void UpdateTime();

	// ��ǰ֡ ��Ⱦ����Ļ���
	void RenderFrame();

	// ������ GUI ��ʵ����Ⱦ �� ��ʱ���¡������� UpdateGUI() ��ע��
	void RenderGUI(const NXSwapChainBuffer& swapChainBuffer);

	void Release();

	NXRenderGraph* GetRenderGraph() { return m_pRenderGraph; }
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

	Ntr<NXReadbackData>& GetVTReadbackData() { return m_vtReadbackData; }

	void NotifyRebuildRenderGraph();

private:
	void InitEvents();
	void InitGlobalResources();
	void GenerateRenderGraph();
	void InitGUI();

	// 2023.11.5 Nix �� GUI ���Ʋ���Ŀǰ��ʱʹ�����ַ�ʽ����ʱ���� �� �ӳٸ���
	// 1. ��ʱ���£�GUI ÿ���޸Ĳ����������������µ���Ӧ����Դ�ϣ�������ͳ�� dearImgui ���²����ķ�����
	// 2. �ӳٸ��£���һ֡�� GUI �޸Ĳ�����ͨ��������е���ʽ���� NXGUICommandManager���ȵ���һ֡ UpdateGUI �ٸ���
	// UpdateGUI() ������ �ӳٸ���
	// RenderGUI() ������GUI����Ⱦ�� ��ʱ����
	void UpdateGUI();

	// ���� NXScene ����
	void UpdateSceneData();

	void OnKeyDown(NXEventArgKey eArg);

	void WaitForBRDF2DLUTFinish();

private:
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		m_pCommandAllocator;
	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	m_pCommandList;

	Vector2				m_viewRTSize;
	NXBRDFLut*			m_pBRDFLut;

	NXScene*			m_scene;

	NXRenderGraph*		m_pRenderGraph;
	bool				m_pNeedRebuildRenderGraph;
	Ntr<NXTexture2D>	m_pFinalRT;

	NXGUI*				m_pGUI;
	bool				m_bRenderGUI;

	bool m_bEnablePostProcessing;
	CBufferColorMapping m_cbColorMappingData;
	NXConstantBuffer<CBufferColorMapping> m_cbColorMapping;

	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
	CBufferDebugLayer m_cbDebugLayerData;
	NXConstantBuffer<CBufferDebugLayer> m_cbDebugLayer;

	Ntr<NXReadbackData> m_vtReadbackData;
};