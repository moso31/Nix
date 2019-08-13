#pragma once
#include "Header.h"

struct SimpleVertex
{
	Vector3 Pos;
	Vector2 Tex;
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
	void Release();

private:
	ID3D11InputLayout*			m_pInputLayout;
	ID3D11VertexShader*			m_pVertexShader;
	ID3D11PixelShader*			m_pPixelShader;
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11Buffer*				m_pConstantBuffer;
	ID3D11ShaderResourceView*	m_pBoxSRV;
	ID3D11SamplerState*			m_pSamplerLinear;

	ConstantBuffer m_boxData;
};