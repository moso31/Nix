#pragma once
#include "ShaderStructures.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"
#include "NXConstantBuffer.h"

enum NXSubMeshReloadState
{
	None,		// 正常状态
	Start,		// A->Default 状态
	Replacing,  // Default->B 状态
	Finish,		// B 状态
};

class NXPrimitive;
class NXSubMeshBase
{
	friend class NXMeshResourceManager;
	friend class NXMaterialResourceManager;
public:
	NXSubMeshBase(NXPrimitive* pPrimitive) : m_pPrimitive(pPrimitive), m_pMaterial(nullptr), m_pReplacingMaterial(nullptr), m_nMatReloadingState(NXSubMeshReloadState::None) {}
	virtual ~NXSubMeshBase() {}

	void Update(ID3D12GraphicsCommandList* pCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pCommandList) = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;
	virtual void TryAddBuffers() = 0;

	// 自动计算顶点的切线数据。
	// bUpdateVBIB: 是否同时更新VBIB
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
	// [Warning!] 不允许直接设置材质！
	// 需使用 BindMaterial() 为场景物体绑定材质
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
	NXSubMesh(NXPrimitive* pPrimitive, const std::string& subMeshName) : NXSubMeshBase(pPrimitive), m_subMeshName(subMeshName) {}
	virtual ~NXSubMesh() {}

	void Render(ID3D12GraphicsCommandList* pCommandList) override;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	virtual void CalculateTangents(bool bUpdateVBIB = false) = 0;
	void TryAddBuffers() override;

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }

	void CalcLocalAABB() override;

protected:
	std::string						m_subMeshName;
	std::vector<TVertex>			m_vertices;
	std::vector<UINT>				m_indices; 
};

class NXSubMeshStandard : public NXSubMesh<VertexPNTT>
{
	friend class FBXMeshLoader;
	friend class NXSubMeshGeometryEditor;
	
public:
	NXSubMeshStandard(NXPrimitive* pPrimitive, const std::string& subMeshName) : NXSubMesh<VertexPNTT>(pPrimitive, subMeshName) {}
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
	NXSubMeshEditorObjects(NXPrimitive* pPrimitive, const std::string& subMeshName, EditorObjectID id);
	virtual ~NXSubMeshEditorObjects() {}

	virtual bool IsSubMeshEditorObject() override { return true; }

	EditorObjectID GetEditorObjectID() { return m_editorObjID; }

	void CalculateTangents(bool bUpdateVBIB = false) override {}
	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;

	void Update(ID3D12GraphicsCommandList* pCmdList, bool isHighLight);

private:
	EditorObjectID m_editorObjID;
	
	ConstantBufferVector4 m_cbDataParams;
	NXConstantBuffer<ConstantBufferVector4>	m_cbParams;
};