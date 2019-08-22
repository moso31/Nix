#pragma once
#include "NXPrimitive.h"

class NXSphere : public NXPrimitive
{
public:
	NXSphere() = default;

	HRESULT Init(float radius, int segmentHorizontal, int segmentVertical);
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

	float m_radius;
	UINT m_segmentVertical;
	UINT m_segmentHorizontal;
};
