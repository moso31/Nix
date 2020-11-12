#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXPassShadowMap
{
public:
	NXPassShadowMap(NXScene* pScene);
	~NXPassShadowMap();

	D3D11_VIEWPORT				GetViewPort()	const	{ return m_viewPort; }
	ID3D11ShaderResourceView*	GetSRV()		const	{ return m_pDepthSRV.Get(); }

	ID3D11Buffer* GetConstantBufferTransform() { return m_cbTransform.Get(); }

	void Init(UINT width, UINT height);
	void UpdateConstantBuffer();
	void Load();
	void Render();
	void Release();

private:
	ComPtr<ID3D11Buffer>				m_cbTransform;
	ConstantBufferShadowMapTransform	m_cbDataTransform;

	ComPtr<ID3D11DepthStencilView>		m_pDepthDSV;
	ComPtr<ID3D11ShaderResourceView>	m_pDepthSRV;
	D3D11_VIEWPORT						m_viewPort;

	NXScene* m_pScene;
};

