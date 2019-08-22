#pragma once
#include "NXPrimitive.h"

class NXCylinder : public NXPrimitive
{
public:
	NXCylinder() = default;

	HRESULT Init(float radius, float length, int segmentCircle, int segmentLength);
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
