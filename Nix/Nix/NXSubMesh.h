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
class NXSubMesh
{
public:
	friend class NXTriangle;
	friend class NXSubMeshGeometryEditor;

public:
	NXSubMesh(NXPrimitive* pPrimitive);
	~NXSubMesh();

	void Update();
	void Render();

	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	NXPrimitive* GetPrimitive() { return m_parent; }

	NXPBRMaterialStandard* GetPBRMaterial() const;
	void SetMaterialPBR(NXPBRMaterialStandard* mat);

	// �Զ����㶥����������ݡ�
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }
	UINT GetFaceCount()		{ return (UINT)m_indices.size() / 3; }

	NXTriangle GetFaceTriangle(UINT faceIndex);
	const VertexPNTT* GetVertexData() { return m_vertices.data(); }

private:
	void InitVertexIndexBuffer();

private:
	NXPrimitive* m_parent;

	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexPNTT>		m_vertices;
	std::vector<UINT>			m_indices;

	NXPBRMaterialStandard*				m_pPBRMaterial;
};
