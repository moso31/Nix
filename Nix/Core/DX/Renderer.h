#pragma once
#include "Header.h"

class Renderer
{
public:
	void Init();
	void InitRenderer();
	void Update();
	void Render();
	void Release();

private:
	ID3D11InputLayout*			m_pInputLayout;
	ID3D11VertexShader*			m_pVertexShader;
	ID3D11PixelShader*			m_pPixelShader;
	ID3D11SamplerState*			m_pSamplerLinear;

	shared_ptr<Scene>			m_scene;
};