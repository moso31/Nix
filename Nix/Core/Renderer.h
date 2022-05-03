#pragma once
#include "Header.h"
#include "GlobalBufferManager.h"

#include "NXDepthPrepass.h"
#include "NXDeferredRenderer.h"
#include "NXForwardRenderer.h"
#include "NXDepthPeelingRenderer.h"
#include "NXSkyRenderer.h"
#include "NXColorMappingRenderer.h"
#include "NXFinalRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"

class Renderer
{
public:
	void Init();
	void InitGUI();
	void InitRenderer();

	// 资源重加载（如果上一帧修改了资源）
	void ResourcesReloading();

	// 更新 NXScene 场景
	void UpdateSceneData();

	// 当前帧 渲染画面的绘制
	void RenderFrame();

	// GUI 的绘制
	void RenderGUI();

	void Release();

private:
	void DrawDepthPrepass();
	void DrawShadowMap();

private:
	ComPtr<ID3D11InputLayout>			m_pInputLayoutP;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPT;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPNT;

	ComPtr<ID3D11VertexShader>			m_pVertexShaderShadowMap;
	ComPtr<ID3D11PixelShader>			m_pPixelShaderShadowMap;

	NXScene*					m_scene;
	NXPassShadowMap*			m_pPassShadowMap; 
	NXDepthPrepass*				m_pDepthPrepass;
	NXDeferredRenderer*			m_pDeferredRenderer;
	NXForwardRenderer*			m_pForwardRenderer;
	NXDepthPeelingRenderer*		m_pDepthPeelingRenderer;
	NXSkyRenderer*				m_pSkyRenderer;
	NXColorMappingRenderer*		m_pColorMappingRenderer;
	NXFinalRenderer*			m_pFinalRenderer;
	NXSimpleSSAO*				m_pSSAO;
	NXGUI* m_pGUI;
};