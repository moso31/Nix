#pragma once
#include "Header.h"
#include "GlobalBufferManager.h"

#include "NXDepthPrepass.h"
#include "NXDeferredRenderer.h"
#include "NXForwardRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"

class Renderer
{
public:
	void Init();
	void InitGUI();
	void InitRenderer();
	void Preload();
	void UpdateSceneData();
	void DrawScene();
	void DrawGUI();
	void Release();

private:
	void DrawDepthPrepass(ID3D11DepthStencilView* pDSVDepth);
	void DrawScene();
	void DrawCubeMap();
	void DrawShadowMap();

private:
	ComPtr<ID3D11InputLayout>			m_pInputLayoutP;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPT;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutPNT;

	ComPtr<ID3D11VertexShader>			m_pVertexShaderCubeMap;
	ComPtr<ID3D11VertexShader>			m_pVertexShaderShadowMap;

	ComPtr<ID3D11PixelShader>			m_pPixelShaderCubeMap;
	ComPtr<ID3D11PixelShader>			m_pPixelShaderShadowMap;

	NXRenderTarget*				m_renderTarget;

	NXScene*					m_scene;
	NXPassShadowMap*			m_pPassShadowMap; 
	NXDepthPrepass*				m_pDepthPrepass;
	NXForwardRenderer*			m_pForwardRenderer;
	NXDeferredRenderer*			m_pDeferredRenderer;
	NXSimpleSSAO*				m_pSSAO;
	NXGUI* m_pGUI;

	// 是否使用延迟着色
	bool m_isDeferredShading;
};