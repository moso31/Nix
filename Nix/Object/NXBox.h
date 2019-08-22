#pragma once
#include "NXPrimitive.h"

class NXBox : public NXPrimitive
{
public:
	NXBox() = default;

	HRESULT Init(float x = 1.0f, float y = 1.0f, float z = 1.0f);
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
