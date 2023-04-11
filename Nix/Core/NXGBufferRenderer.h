#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXGBufferRenderer
{
private:
	explicit NXGBufferRenderer() = default;
public:
	NXGBufferRenderer(NXScene* pScene);
	~NXGBufferRenderer();

	void Init();
	void Render();

	void Release();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilStateSSS;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	ComPtr<ID3D11SamplerState>			m_pSamplerLinearWrap;
	ComPtr<ID3D11SamplerState>			m_pSamplerLinearClamp;

	NXScene* m_pScene;
	NXCustomMaterial* m_pTestCustomMat;
};
