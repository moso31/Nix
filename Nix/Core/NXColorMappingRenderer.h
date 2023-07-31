#pragma once
#include "header.h"
#include "ShaderStructures.h"

struct CBufferColorMapping
{
	Vector4 param0; // x: enable
};

class NXColorMappingRenderer
{
public:
	NXColorMappingRenderer();
	~NXColorMappingRenderer();

	void Init();
	void Render();

	bool GetEnable() const { return m_bEnablePostProcessing; }
	void SetEnable(bool value) { m_bEnablePostProcessing = value; }

	void Release();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXRenderTarget* m_pFinalRT;

private:
	bool m_bEnablePostProcessing;

	ComPtr<ID3D11Buffer>				m_cbParams;
	CBufferColorMapping					m_cbDataParams;
};
