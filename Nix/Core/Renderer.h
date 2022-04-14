#pragma once
#include "Header.h"
#include "GlobalBufferManager.h"

#include "NXDepthPrepass.h"
#include "NXDeferredRenderer.h"
#include "NXForwardRenderer.h"
#include "NXSkyRenderer.h"
#include "NXSimpleSSAO.h"
#include "NXGUI.h"

class Renderer
{
public:
	void Init();
	void InitGUI();
	void InitRenderer();

	// ��Դ�ؼ��أ������һ֡�޸�����Դ��
	void ResourcesReloading();

	// ���� NXScene ����
	void UpdateSceneData();

	// ��ǰ֡ ��Ⱦ����Ļ���
	void RenderFrame();

	// GUI �Ļ���
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

	NXRenderTarget*				m_renderTarget;

	NXScene*					m_scene;
	NXPassShadowMap*			m_pPassShadowMap; 
	NXDepthPrepass*				m_pDepthPrepass;
	NXForwardRenderer*			m_pForwardRenderer;
	NXDeferredRenderer*			m_pDeferredRenderer;
	NXSkyRenderer*				m_pSkyRenderer;
	NXSimpleSSAO*				m_pSSAO;
	NXGUI* m_pGUI;
};