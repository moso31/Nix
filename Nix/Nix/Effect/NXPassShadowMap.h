#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXPassShadowMap
{
public:
	NXPassShadowMap(const shared_ptr<Scene>& pScene);
	~NXPassShadowMap();

	D3D11_VIEWPORT				GetViewPort()	const	{ return m_viewPort; }
	ID3D11ShaderResourceView*	GetSRV()		const	{ return m_pDepthSRV; }

	void SetConstantBufferTransform(const ConstantBufferShadowMapTransform& cbDataVP);
	void SetConstantBufferWorld(const ConstantBufferPrimitive& cbDataWorld);
	ID3D11Buffer* GetConstantBufferTransform() { return m_cbTransform; }

	void Init(UINT width, UINT height);
	void UpdateConstantBuffer();
	void Load();
	void Render();
	void Release();

private:
	ID3D11Buffer*						m_cbTransform;
	ConstantBufferShadowMapTransform	m_cbDataTransform;

	ID3D11DepthStencilView*			m_pDepthDSV;
	ID3D11ShaderResourceView1*		m_pDepthSRV;
	D3D11_VIEWPORT					m_viewPort;

	shared_ptr<Scene>				m_pScene;
};

