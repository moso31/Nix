#pragma once

class NXSimpleSSAO
{
public:
	NXSimpleSSAO();
	~NXSimpleSSAO();

	void Init();
	void Render();

private:
	//ComPtr<ID3D11VertexShader>			m_pVertexShader;
	//ComPtr<ID3D11PixelShader>			m_pPixelShader;
	//ComPtr<ID3D11InputLayout>			m_pInputLayout;
};
