#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXBox;
class NXSphere;
class NXCylinder;
class NXCone;
class NXPlane;

class NXHit;
class NXPBRMaterial;

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	virtual ~NXPrimitive() {}

	friend class NXTriangle;

	virtual void Update();
	virtual void Render();
	virtual void Release();

	void SetMaterial(const shared_ptr<NXMaterial>& material);

	shared_ptr<NXPBRMaterial> GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	virtual bool Intersect(const Ray& ray, Vector3& outHitPos, float& outDist);
	virtual bool RayCast(const Ray& localRay, NXHit& outHitInfo);

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
	shared_ptr<NXPBRMaterial>	m_pPBRMaterial;

	AABB m_aabb;
};

class NXTriangle
{
public:
	NXTriangle(Vector3 p0, Vector3 p1, Vector3 p2) : p0(p0), p1(p1), p2(p2) {}
	NXTriangle(const shared_ptr<NXPrimitive>& pShape, int startIndex);
	~NXTriangle() {};

	bool RayCast(const Ray& localRay, NXHit& outHitInfo);

private:
	Vector3 p0, p1, p2;
	shared_ptr<NXPrimitive> pShape;
};
