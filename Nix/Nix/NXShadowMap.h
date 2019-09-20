#pragma once
#include "ShaderStructures.h"

class NXShadowMap
{
public:
	NXShadowMap();
	~NXShadowMap();

	D3D11_VIEWPORT				GetViewPort()	const	{ return m_viewPort; }
	ID3D11ShaderResourceView*	GetSRV()		const	{ return m_pDepthSRV; }

	void UpdateConstantBuffer(const Matrix& viewMatrix, const Matrix& projMatrix);

	void Init(UINT width, UINT height);
	void Update();
	void Render(const shared_ptr<Scene>& pTargetScene);
	void Release();

private:
	ID3D11DepthStencilView*		m_pDepthDSV;
	ID3D11ShaderResourceView1*	m_pDepthSRV;

	D3D11_VIEWPORT m_viewPort;

	ID3D11Buffer*					m_cbShadowMapCamera;
	ConstantBufferShadowMapCamera	m_cbShadowMapCameraData;
};
