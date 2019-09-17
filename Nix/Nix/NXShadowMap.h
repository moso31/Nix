#pragma once
#include "Header.h"

class NXShadowMap
{
public:
	NXShadowMap();
	~NXShadowMap();

	void Init(UINT width, UINT height);
	void Update();
	void Render();

private:
	ID3D11DepthStencilView*		m_pDepthDSV;
	ID3D11ShaderResourceView1*	m_pDepthSRV;
};
