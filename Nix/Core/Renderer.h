#pragma once
#include "Header.h"
#include "GlobalBufferManager.h"
#include "NXBRDFlut.h"

#include "NXDepthPrepass.h"
#include "NXGBufferRenderer.h"
#include "NXShadowMapRenderer.h"
#include "NXShadowTestRenderer.h"
#include "NXDeferredRenderer.h"
#include "NXForwardRenderer.h"
#include "NXDepthPeelingRenderer.h"
#include "NXSkyRenderer.h"
#include "NXColorMappingRenderer.h"
#include "NXFinalRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"
#include "NXDebugLayerRenderer.h"
#include "NXEditorObjectRenderer.h"

struct NXEventArgKey;
class Renderer
{
public:
	Renderer();

	void Init();
	void InitGUI();
	void InitRenderer();
	void InitEvents();

	// ��Դ�ؼ��أ������һ֡�޸�����Դ��
	void ResourcesReloading();

	// ��Ⱦ�����ؼ���
	void PipelineReloading();

	// ���� NXScene ����
	void UpdateSceneData();

	// ��ǰ֡ ��Ⱦ����Ļ���
	void RenderFrame();

	// GUI �Ļ���
	void RenderGUI();

	void Release();

public:
	NXSimpleSSAO*			GetSSAORenderer()			{ return m_pSSAO; }
	NXShadowMapRenderer*	GetShadowMapRenderer()		{ return m_pShadowMapRenderer; }
	// 2023.3.10 Ŀǰ PostProcessing ֻ�� ColorMapping���� ������������ʱ����ͬ��ʡ�
	NXColorMappingRenderer* GetColorMappingRenderer()   { return m_pColorMappingRenderer; }
	NXDebugLayerRenderer*	GetDebugLayerRenderer()		{ return m_pDebugLayerRenderer; }

private:
	void DrawDepthPrepass();
	void OnKeyDown(NXEventArgKey eArg);

private:
	ComPtr<ID3D11InputLayout>			m_pInputLayoutP;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPT;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPNT;

	NXBRDFLut*							m_pBRDFLut;

	NXScene*							m_scene;
	NXDepthPrepass*						m_pDepthPrepass;
	NXGBufferRenderer*					m_pGBufferRenderer;
	NXShadowMapRenderer*				m_pShadowMapRenderer;
	NXShadowTestRenderer*				m_pShadowTestRenderer;
	NXDeferredRenderer*					m_pDeferredRenderer;
	NXForwardRenderer*					m_pForwardRenderer;
	NXDepthPeelingRenderer*				m_pDepthPeelingRenderer;
	NXSkyRenderer*						m_pSkyRenderer;
	NXColorMappingRenderer*				m_pColorMappingRenderer;
	NXFinalRenderer*					m_pFinalRenderer;
	NXSimpleSSAO*						m_pSSAO;
	NXDebugLayerRenderer*				m_pDebugLayerRenderer;
	NXEditorObjectRenderer*				m_pEditorObjectRenderer;

	NXGUI*								m_pGUI;

	bool								m_bRenderGUI;
};