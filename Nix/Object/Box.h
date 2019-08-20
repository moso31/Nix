#pragma once
#include "Primitive.h"

class Box : public Primitive
{
public:
	Box() = default;

	HRESULT Init();
	void Update();
	void Render();
	void Release();

	void SetMaterial(const shared_ptr<Material> pMaterial);

private:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11Buffer*				m_pConstantBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;

	ConstantBufferPrimitive		m_pConstantBufferData;

	shared_ptr<Material>		m_pMaterial;
};
