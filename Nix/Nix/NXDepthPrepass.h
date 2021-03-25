#pragma once
#include "Header.h"

class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init();
	void Render();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	NXScene* m_pScene;
	ComPtr<ID3D11Texture2D>				m_pTexBRDF2DLUT;
};
