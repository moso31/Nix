#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXBox;
class NXSphere;
class NXCylinder;
class NXCone;
class NXPlane;

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	~NXPrimitive();

	virtual void Update();
	virtual void Render() = 0;
	virtual void Release() = 0;

	void SetMaterial(const shared_ptr<NXMaterial>& material);

protected:
	void InitVertexIndexBuffer();

protected:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11Buffer*				m_pConstantBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;

	ConstantBufferPrimitive		m_pConstantBufferData;

	ConstantBufferMaterial		m_cbDataMaterial;
	ID3D11Buffer*				m_cbMaterial;
	shared_ptr<NXMaterial>		m_pMaterial;
};
