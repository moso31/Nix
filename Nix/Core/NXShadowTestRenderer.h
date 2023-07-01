#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXShadowTestRenderer
{
public:
	NXShadowTestRenderer() = default;
	~NXShadowTestRenderer();

	void Init();
	void Render(NXTexture2DArray* pShadowMapDepthTex);

	void Release();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXRenderTarget* m_pResultRT;
};
