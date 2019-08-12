#pragma once
#include "Header.h"

struct SimpleVertex
{
	Vector3 Pos;
};

class Renderer
{
public:
	HRESULT InitRenderer();
	void Render();

private:
	ID3D11InputLayout*		m_pInputLayout;
	ID3D11VertexShader*		m_pVertexShader;
	ID3D11PixelShader*		m_pPixelShader;

	ID3D11Buffer*           m_pVertexBuffer;
};