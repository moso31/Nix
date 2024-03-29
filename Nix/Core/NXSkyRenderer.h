#pragma once
#include "BaseDefs/DX11.h"
#include "ShaderStructures.h"

class NXScene;
class NXSkyRenderer
{
public:
	NXSkyRenderer(NXScene* pScene);
	~NXSkyRenderer();

	void Init();
	void Render();

	void Release() {}

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXScene* m_pScene;
};
