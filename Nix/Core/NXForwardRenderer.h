#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXForwardRenderer
{
public:
	NXForwardRenderer(NXScene* pScene);
	~NXForwardRenderer();

	void Init();
	void Render(ID3D11ShaderResourceView* pSRVSSAO);

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	NXScene* m_pScene;
};
