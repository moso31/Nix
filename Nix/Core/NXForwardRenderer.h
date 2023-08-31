#pragma once
#include "BaseDefs/DX11.h"
#include "ShaderStructures.h"

class NXScene;
class NXBRDFLut;
class NXForwardRenderer
{
public:
	NXForwardRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXForwardRenderer();

	void Init();
	void Render();

	void Release() {};

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
