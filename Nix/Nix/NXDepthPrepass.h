#pragma once
#include "Header.h"

class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init(const Vector2& DepthBufferSize);
	void Render();

	ID3D11ShaderResourceView*		GetSRVNormal()			{ return m_pSRVNormal.Get(); }

	ID3D11ShaderResourceView*		GetSRVDepthPrepass()	{ return m_pSRVDepthPrepass.Get(); }
	ID3D11DepthStencilView*			GetDSVDepthPrepass()	{ return m_pDSVDepthPrepass.Get(); }

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11ShaderResourceView>	m_pSRVNormal;

	ComPtr<ID3D11Texture2D>				m_pTexDepthPrepass;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVDepthPrepass;
	ComPtr<ID3D11DepthStencilView>		m_pDSVDepthPrepass;

	NXScene* m_pScene;
};
