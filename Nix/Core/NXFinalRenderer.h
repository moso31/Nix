#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXFinalRenderer
{
public:
	NXFinalRenderer(ID3D11RenderTargetView* pRTVFinalQuad);
	~NXFinalRenderer();

	void Init();
	void OnResize(ID3D11RenderTargetView* pRTVFinalQuad);
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

	// 最终渲染的FinalRT临时用一张原生RTV存储
	ID3D11RenderTargetView* m_pRTVFinalQuad;

	NXRenderTarget* m_pFinalRT;

	NXTexture2D* m_pInputTexture;
};
