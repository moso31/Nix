#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"
#include "NXIntersection.h"

class NXPBRMaterial;

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	~NXPrimitive() {}

	friend class NXTriangle;

	virtual void Update();
	virtual void Render();
	virtual void Release();

	// 自动计算顶点的切线数据。
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	void SetMaterialPBR(NXPBRMaterial* mat);

	NXPBRMaterial* GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial.Get(); }

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

protected:
	virtual void InitVertexIndexBuffer();
	void InitMaterialBuffer();
	void InitAABB();

protected:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexPNTT>		m_vertices;
	std::vector<UINT>			m_indices;
	std::vector<Vector3>		m_points;	// vertices position 序列

	ConstantBufferMaterial		m_cbDataMaterial;
	ComPtr<ID3D11Buffer>		m_cbMaterial;
	NXPBRMaterial*				m_pPBRMaterial;

	AABB m_aabb;
};

class NXTriangle
{
public:
	NXTriangle(NXPrimitive* pShape, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNTT GetVertex(int VertexId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	NXPrimitive* pShape;
};
