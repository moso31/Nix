#pragma once
#include "Header.h"

class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init(const Vector2& DepthBufferSize);
	void Render(ID3D11DepthStencilView* pDSVDepth);

	ID3D11ShaderResourceView*		GetSRVNormal()		{ return m_pSRVNormal.Get(); }

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11Texture2D>				m_pTexNormal;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVNormal;

	NXScene* m_pScene;
};
