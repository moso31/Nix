#pragma once
#include "Header.h"

class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init(const Vector2& DepthBufferSize);
	void Render(ID3D11DepthStencilView* pDSVDepth);

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11Texture2D>				m_pTexDepth;
	ComPtr<ID3D11RenderTargetView>		m_pRTVDepth;

	NXScene* m_pScene;
};
