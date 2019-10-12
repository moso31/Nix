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
	virtual ~NXPrimitive() {}

	virtual void Update();
	virtual void Render();
	virtual void Release();

	void SetMaterial(const shared_ptr<NXMaterial>& material);
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	virtual bool Intersect(const Ray& Ray, Vector3& outHitPos, float& outDist);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

protected:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;
	vector<Vector3>				m_points;	// vertices position ађСа

	ConstantBufferMaterial		m_cbDataMaterial;
	ID3D11Buffer*				m_cbMaterial;
	shared_ptr<NXMaterial>		m_pMaterial;

	AABB m_aabb;
};
