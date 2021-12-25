#pragma once
#include "Header.h"

class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init(const Vector2& DepthBufferSize);
	void Render();

private:
	ComPtr<ID3D11InputLayout>			m_pInputLayout;
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;

	NXScene* m_pScene;
};
