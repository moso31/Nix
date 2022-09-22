#pragma once
#include "ShaderStructures.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"

class NXTriangle
{
public:
	NXTriangle(NXSubMesh* pSubMesh, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNTT GetVertex(int VertexId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	NXSubMesh* pSubMesh;
};

class NXPrimitive;

class NXSubMeshBase
{
public:
	NXSubMeshBase(NXPrimitive* pParent) : m_pParent(pParent), m_pMaterial(nullptr) {}
	~NXSubMeshBase() {}

	void UpdateViewParams();

	void Update();
	void Render();

	NXMaterial* GetMaterial() const { return m_pMaterial; }
	void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

protected:
	NXPrimitive* m_pParent;
	NXMaterial* m_pMaterial;
};

template<class VertexType>
class NXSubMesh : public NXSubMeshBase
{
	friend class NXTriangle;
	friend class NXSubMeshGeometryEditor;
	friend class FBXMeshLoader;
	friend class SceneManager;

public:
	NXSubMesh(NXPrimitive* pPrimitive);
	~NXSubMesh();

	void UpdateViewParams();

	void Update();
	void Render();

	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	NXPrimitive* GetPrimitive() { return m_parent; }

	NXMaterial* GetMaterial() const { return m_pMaterial; }

	// 自动计算顶点的切线数据。
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }
	UINT GetFaceCount()		{ return (UINT)m_indices.size() / 3; }

	NXTriangle GetFaceTriangle(UINT faceIndex);
	const VertexPNTT* GetVertexData() { return m_vertices.data(); }

private:
	void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

private:
	void InitVertexIndexBuffer();

private:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexPNTT>		m_vertices;
	std::vector<UINT>			m_indices;
};
