#pragma once
#include "BaseDefs/DX11.h"
#include "BaseDefs/Math.h"

class NXScene;
class NXDepthPrepass
{
public:
	NXDepthPrepass(NXScene* pScene);
	~NXDepthPrepass();

	void Init();
	void OnResize(const Vector2& rtSize);
	void Render();

private:
	ComPtr<ID3D11InputLayout>			m_pInputLayout;
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;

	NXScene* m_pScene;
};
