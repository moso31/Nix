#pragma once
#include "Header.h"

struct SimpleVertex
{
	Vector3 Pos;
	Vector4 Color;
};

struct ConstantBuffer
{
	Matrix mWorld;
	Matrix mView;
	Matrix mProjection;
};

class Renderer
{
public:
	HRESULT InitRenderer();
	void Update();
	void Render();

private:
	ID3D11InputLayout*		m_pInputLayout;
	ID3D11VertexShader*		m_pVertexShader;
	ID3D11PixelShader*		m_pPixelShader;

	ID3D11Buffer*           m_pVertexBuffer;
	ID3D11Buffer*			m_pIndexBuffer;
	ID3D11Buffer*			m_pConstantBuffer;

	ConstantBuffer m_boxData;
};