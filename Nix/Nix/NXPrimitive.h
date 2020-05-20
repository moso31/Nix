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
	void SetMaterialPBR(const shared_ptr<NXPBRMaterial>& mat);

	shared_ptr<NXPBRMaterial> GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	//VertexPNT GetVertexPNT() { return }

	virtual bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

protected:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;
	vector<Vector3>				m_points;	// vertices position ����

	ConstantBufferMaterial		m_cbDataMaterial;
	ID3D11Buffer*				m_cbMaterial;
	shared_ptr<NXMaterial>		m_pMaterial;
	shared_ptr<NXPBRMaterial>	m_pPBRMaterial;

	AABB m_aabb;
};

class NXTriangle
{
public:
	NXTriangle(const shared_ptr<NXPrimitive>& pShape, int startIndex);
	~NXTriangle() {};

	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	shared_ptr<NXPrimitive> pShape;
};
