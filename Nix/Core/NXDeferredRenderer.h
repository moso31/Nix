#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXDeferredRenderer
{
public:
	NXDeferredRenderer(NXScene* pScene);
	~NXDeferredRenderer();

	void Init();
	void RenderGBuffer();
	void Render(ID3D11ShaderResourceView* pSRVSSAO);
	void RenderRT0();
	void RenderRT1();
	void RenderRT2();
	void RenderRT3();

private:
	void InitVertexIndexBuffer();

private:
	ComPtr<ID3D11Texture2D>				m_pTex[4];
	ComPtr<ID3D11ShaderResourceView>	m_pSRV[4];
	ComPtr<ID3D11RenderTargetView>		m_pRTV[4];

	ComPtr<ID3D11Texture2D>				m_pTexDepth;
	ComPtr<ID3D11DepthStencilView>		m_pDSVDepth;

	std::vector<VertexPT>				m_vertices;
	std::vector<UINT>					m_indices;

	ComPtr<ID3D11InputLayout>			m_pInputLayoutGBuffer;
	ComPtr<ID3D11InputLayout>			m_pInputLayoutRender;

	ComPtr<ID3D11Buffer>				m_pVertexBuffer;
	ComPtr<ID3D11Buffer>				m_pIndexBuffer;

	// ��ȾRT0-RT4ʱʹ�ô���Shader
	ComPtr<ID3D11VertexShader>			m_pVertexShader; 
	ComPtr<ID3D11PixelShader>			m_pPixelShader2;

	// ����������Ⱦʱʹ�ô���Shaders
	ComPtr<ID3D11VertexShader>			m_pVertexShaderRender; 
	ComPtr<ID3D11PixelShader>			m_pPixelShaderRender;

	NXScene* m_pScene;
};
