#pragma once
#include "ShaderStructures.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"

class NXPrimitive;
class NXSubMeshBase
{
public:
	NXSubMeshBase(NXPrimitive* pPrimitive) : m_pPrimitive(pPrimitive), m_pMaterial(nullptr) {}
	virtual ~NXSubMeshBase() {}

	void UpdateViewParams();

	void Update() { if (m_pMaterial) m_pMaterial->Update(); }
	virtual void Render() = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;
	virtual void UpdateVBIB() = 0;

	// 自动计算顶点的切线数据。
	// bUpdateVBIB: 是否同时更新VBIB
	virtual void CalculateTangents(bool bUpdateVBIB) = 0;

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
public:
	NXSubMesh(NXPrimitive* pPrimitive) : NXSubMeshBase(pPrimitive) {}
	virtual ~NXSubMesh() {}

	void Render() override;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	virtual void CalculateTangents(bool bUpdateVBIB = false) = 0;
	void UpdateVBIB() override;

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }

	void CalcLocalAABB() override;

protected:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<TVertex>		m_vertices;
	std::vector<UINT>			m_indices;
};

class NXSubMeshStandard : public NXSubMesh<VertexPNTT>
{
	friend class FBXMeshLoader;
	friend class NXSubMeshGeometryEditor;
	
public:
	NXSubMeshStandard(NXPrimitive* pPrimitive) : NXSubMesh<VertexPNTT>(pPrimitive) {}
	virtual ~NXSubMeshStandard() {}

	void CalculateTangents(bool bUpdateVBIB = false) override;
};

class NXSubMeshEditorObjects : public NXSubMesh<VertexEditorObjects>
{
	friend class NXSubMeshGeometryEditor;

public:
	enum EditorObjectID
	{
		NONE,
		TRANSLATE_X,
		TRANSLATE_Y,
		TRANSLATE_Z,
		TRANSLATE_XY,
		TRANSLATE_XZ,
		TRANSLATE_YZ,
		MAX
	};

	NXSubMeshEditorObjects(NXPrimitive* pPrimitive, EditorObjectID id) : NXSubMesh<VertexEditorObjects>(pPrimitive), m_editorObjID(id) {}
	virtual ~NXSubMeshEditorObjects() {}

	EditorObjectID GetEditorObjectID() { return m_editorObjID; }

	void CalculateTangents(bool bUpdateVBIB = false) override {}
	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;

private:
	EditorObjectID m_editorObjID;
};