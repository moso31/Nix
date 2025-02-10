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

	// ��Դ�ؼ��أ������һ֡�޸�����Դ��
	void ResourcesReloading();

	void Update();

	void UpdateTime();

	// ��ǰ֡ ��Ⱦ����Ļ���
	void RenderFrame();

	// ������ GUI ��ʵ����Ⱦ �� ��ʱ���¡������� UpdateGUI() ��ע��
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

	// 2023.11.5 Nix �� GUI ���Ʋ���Ŀǰ��ʱʹ�����ַ�ʽ����ʱ���� �� �ӳٸ���
	// 1. ��ʱ���£�GUI ÿ���޸Ĳ����������������µ���Ӧ����Դ�ϣ�������ͳ�� dearImgui ���²����ķ�����
	// 2. �ӳٸ��£���һ֡�� GUI �޸Ĳ�����ͨ��������е���ʽ���� NXGUICommandManager���ȵ���һ֡ UpdateGUI �ٸ���
	// UpdateGUI() ������ �ӳٸ���
	// RenderGUI() ������GUI����Ⱦ�� ��ʱ����
	void UpdateGUI();

	// ���� NXScene ����
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