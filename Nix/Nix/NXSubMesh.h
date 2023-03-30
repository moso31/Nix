#pragma once
#include "ShaderStructures.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"

enum NXSubMeshReloadState
{
	None,		// ����״̬
	Start,		// A->Default ״̬
	Replacing,  // Default->B ״̬
	Finish,		// B ״̬
};

class NXPrimitive;
class NXSubMeshBase
{
	friend class NXMeshResourceManager;
	friend class NXMaterialResourceManager;
public:
	NXSubMeshBase(NXPrimitive* pPrimitive) : m_pPrimitive(pPrimitive), m_pMaterial(nullptr), m_pReplacingMaterial(nullptr), m_nMatReloadingState(NXSubMeshReloadState::None) {}
	virtual ~NXSubMeshBase() {}

	void UpdateViewParams();

	void Update() { if (m_pMaterial) m_pMaterial->Update(); }
	virtual void Render() = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;
	virtual void UpdateVBIB() = 0;

	// �Զ����㶥����������ݡ�
	// bUpdateVBIB: �Ƿ�ͬʱ����VBIB
	virtual void CalculateTangents(bool bUpdateVBIB) = 0;

	NXPrimitive* GetPrimitive() { return m_pPrimitive; }
	NXMaterial* GetMaterial() const { return m_pMaterial; }

	virtual void CalcLocalAABB() = 0;
	AABB GetLocalAABB() { return m_localAABB; }

	virtual bool IsSubMeshStandard()		{ return false; }
	virtual bool IsSubMeshEditorObject()	{ return false; }

	void MarkReplacing(const std::filesystem::path& replaceMaterialPath);
	void SwitchToLoadingMaterial();
	void SwitchToReplacingMaterial();
	void OnReplaceFinish();

	NXSubMeshReloadState GetReloadingState() { return m_nMatReloadingState; }
	void SetReloadingState(NXSubMeshReloadState state) { m_nMatReloadingState = state; }

private:
	// [Warning!] ������ֱ�����ò��ʣ�
	// ��ʹ�� BindMaterial() Ϊ��������󶨲���
	void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

protected:
	NXPrimitive* m_pPrimitive;
	NXMaterial* m_pMaterial;

	AABB m_localAABB;

	NXSubMeshReloadState m_nMatReloadingState;
	std::filesystem::path m_strReplacingPath;
	NXMaterial* m_pReplacingMaterial;
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

	virtual bool IsSubMeshStandard()	 override { return true; }

	void CalculateTangents(bool bUpdateVBIB = false) override;
};

/////////////////////////////////////////////////////////////////////////////
// Editor Objects
/////////////////////////////////////////////////////////////////////////////

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

class NXSubMeshEditorObjects : public NXSubMesh<VertexEditorObjects>
{
	friend class NXSubMeshGeometryEditor;

public:
	NXSubMeshEditorObjects(NXPrimitive* pPrimitive, EditorObjectID id) : NXSubMesh<VertexEditorObjects>(pPrimitive), m_editorObjID(id) {}
	virtual ~NXSubMeshEditorObjects() {}

	virtual bool IsSubMeshEditorObject() override { return true; }

	EditorObjectID GetEditorObjectID() { return m_editorObjID; }

	void CalculateTangents(bool bUpdateVBIB = false) override {}
	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;

private:
	EditorObjectID m_editorObjID;
};