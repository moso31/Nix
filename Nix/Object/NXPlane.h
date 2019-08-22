#pragma once
#include "NXPrimitive.h"

class NXPlane : public NXPrimitive
{
public:
	NXPlane() = default;

	HRESULT Init(float width = 0.5f, float height = 0.5f);
	void Update();
	void Render();
	void Release();

private:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11Buffer*				m_pConstantBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;

	ConstantBufferPrimitive		m_pConstantBufferData;
};
