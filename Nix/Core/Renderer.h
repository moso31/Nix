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

	// 资源重加载（如果上一帧修改了资源）
	void ResourcesReloading();

	// 渲染管线重加载
	void PipelineReloading();

	// 更新 NXScene 场景
	void UpdateSceneData();

	// 当前帧 渲染画面的绘制
	void RenderFrame();

	// GUI 的绘制
	void RenderGUI();

	void Release();

public:
	NXSimpleSSAO*			GetSSAORenderer()			{ return m_pSSAO; }
	NXShadowMapRenderer*	GetShadowMapRenderer()		{ return m_pShadowMapRenderer; }
	// 2023.3.10 目前 PostProcessing 只有 ColorMapping…… 所以这两个暂时算是同义词。
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