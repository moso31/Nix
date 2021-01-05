#pragma once
#include "NXPrimitive.h"

class NXRenderTarget : public NXPrimitive
{
public:
	NXRenderTarget();
	~NXRenderTarget() {}

	void Init();
	void Render();

private:
	void InitVertexIndexBuffer() override;
	void InitRenderData();

private:
	std::vector<VertexPT>		m_vertices;

	ComPtr<ID3D11VertexShader>	m_pVertexShader;
	ComPtr<ID3D11PixelShader>	m_pPixelShader;
	ComPtr<ID3D11InputLayout>	m_pInputLayout;
};

