#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXColorMappingRenderer;

class NXEditorObjectRenderer
{
public:
	NXEditorObjectRenderer(NXScene* pScene);
	~NXEditorObjectRenderer();

	void Init();
	void Render();

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

	// ��2022.9.20 ��ȡ��ǰpass����������������DebugLayer��DebugLayerTex��һ���¡���ͷͳһһ�¡���
	NXTexture2D* GetPassOutTex() { return m_pPassOutTex; }

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXScene*							m_pScene;
	NXRenderTarget*						m_pRTQuad;
	
	// pass output resources
	NXTexture2D*						m_pPassOutTex;

	bool m_bEnable;
};
