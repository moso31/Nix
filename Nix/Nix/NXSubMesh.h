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
	NXSubMeshBase(NXRenderableObject* pRenderableObject) : m_pRenderableObject(pRenderableObject), m_pMaterial(nullptr), m_pReplacingMaterial(nullptr), m_nMatReloadingState(NXSubMeshReloadState::None) {}
	virtual ~NXSubMeshBase() {}

	void Update(ID3D12GraphicsCommandList* pCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pCommandList) = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;
	virtual void TryAddBuffers() = 0;

	// 自动计算顶点的切线数据。
	// bUpdateVBIB: 是否同时更新VBIB
	virtual void CalculateTangents(bool bUpdateVBIB) = 0;

	NXRenderableObject* GetRenderableObject() { return m_pRenderableObject; }
	NXMaterial* GetMaterial() const { return m_pMaterial; }

	virtual void CalcLocalAABB() = 0;
	AABB GetLocalAABB() { return m_localAABB; }

	virtual bool IsSubMeshStandard()		{ return false; }
	virtual bool IsSubMeshTerrain()			{ return false; }
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
	NXRenderableObject* m_pRenderableObject;
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
	NXSubMesh(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMeshBase(pRenderableObject), m_subMeshName(subMeshName) {}
	virtual ~NXSubMesh() {}

	void AppendVertices(std::vector<TVertex>&& vertices) { m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end()); }
	void AppendIndices(std::vector<UINT>&& indices) { m_indices.insert(m_indices.end(), indices.begin(), indices.end()); }

	void Render(ID3D12GraphicsCommandList* pCommandList) override;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	virtual void CalculateTangents(bool bUpdateVBIB = false) {}
	virtual void TryAddBuffers() override;

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }

	void CalcLocalAABB() override;

protected:
	std::string						m_subMeshName;
	std::vector<TVertex>			m_vertices;
	std::vector<UINT>				m_indices; 
};

template<class TVertex, class TInstanceData>
class NXSubMeshInstanced : public NXSubMesh<TVertex>
{
public:
	NXSubMeshInstanced(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMesh<TVertex>(pRenderableObject, subMeshName) {}
	virtual ~NXSubMeshInstanced() {}

	void AppendInstanceData(std::vector<TInstanceData>&& instanceData) { m_instanceData.insert(m_instanceData.end(), instanceData.begin(), instanceData.end()); }
	virtual void TryAddBuffers() override;

	void Render(ID3D12GraphicsCommandList* pCmdList) override;
	uint32_t GetInstanceCount() { return (uint32_t)m_instanceData.size(); }

protected:
	std::vector<TInstanceData>	m_instanceData;
};

class NXSubMeshStandard : public NXSubMesh<VertexPNTT>
{
public:
	NXSubMeshStandard(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMesh<VertexPNTT>(pRenderableObject, subMeshName) {}
	virtual ~NXSubMeshStandard() {}

	virtual bool IsSubMeshStandard()	 override { return true; }

	void CalculateTangents(bool bUpdateVBIB = false) override;
};

class NXSubMeshTerrain : public NXSubMeshInstanced<VertexPNTC, InstanceData>
{
public:
	NXSubMeshTerrain(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMeshInstanced<VertexPNTC, InstanceData>(pRenderableObject, subMeshName) {}
	virtual ~NXSubMeshTerrain() {}

	virtual bool IsSubMeshTerrain()		 override { return true; }
};

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
public:
	NXSubMeshEditorObjects(NXRenderableObject* pRenderableObject, const std::string& subMeshName, EditorObjectID id);
	virtual ~NXSubMeshEditorObjects() {}

	virtual bool IsSubMeshEditorObject() override { return true; }

	EditorObjectID GetEditorObjectID() { return m_editorObjID; }

	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;

	void Update(ID3D12GraphicsCommandList* pCmdList, bool isHighLight);

private:
	EditorObjectID m_editorObjID;
	
	ConstantBufferVector4 m_cbDataParams;
	NXConstantBuffer<ConstantBufferVector4>	m_cbParams;
};
