#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXFinalRenderer
{
public:
	NXFinalRenderer();
	~NXFinalRenderer();

	void Init();
	void OnResize();
	void Render();

	void Release();

	void SetInputTexture(NXTexture2D* pInputTexture);
	NXTexture2D* GetInputTexture() { return m_pInputTexture; }

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	ComPtr<ID3D11SamplerState>			m_pSamplerLinearClamp;

	NXRenderTarget* m_pFinalRT;

	NXTexture2D* m_pInputTexture;
};
