#pragma once
#include "ShaderStructures.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"

class NXPrimitive;
class NXSubMeshBase
{
public:
	NXSubMeshBase(NXPrimitive* pPrimitive) : m_pPrimitive(pPrimitive), m_pMaterial(nullptr) {}
	~NXSubMeshBase() {}

	void UpdateViewParams();

	void Update() { if (m_pMaterial) m_pMaterial->Update(); }
	virtual void Render() = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;

	// 自动计算顶点的切线数据。
	// bUpdateVBIB: 是否同时更新VBIB
	virtual void CalculateTangents(bool bUpdateVBIB) = 0;
	virtual void UpdateVBIB() = 0;

	NXPrimitive* GetPrimitive() { return m_pPrimitive; }
	NXMaterial* GetMaterial() const { return m_pMaterial; }
	void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

	virtual void CalcLocalAABB() = 0;
	AABB GetLocalAABB() { return m_localAABB; }

protected:
	NXPrimitive* m_pPrimitive;
	NXMaterial* m_pMaterial;

	AABB m_localAABB;
};

template<class TVertex>
class NXSubMesh : public NXSubMeshBase
{
	friend class FBXMeshLoader;
	friend class NXSubMeshGeometryEditor;
public:
	NXSubMesh(NXPrimitive* pPrimitive) : NXSubMeshBase(pPrimitive) {}
	~NXSubMesh() {}

	void Render() override;

	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	void CalculateTangents(bool bUpdateVBIB = false);

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }

	const TVertex* GetVertexData() { return m_vertices.data(); }

	void CalcLocalAABB() override;

private:
	void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

private:
	void UpdateVBIB() override;

private:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<TVertex>		m_vertices;
	std::vector<UINT>			m_indices;
};
