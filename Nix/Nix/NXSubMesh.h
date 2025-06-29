#pragma once
#include "ShaderStructures.h"
#include "NXSubMeshCommon.h"
#include "NXPBRMaterial.h"
#include "NXIntersection.h"
#include "NXConstantBuffer.h"
#include "NXSubMeshGeometryEditor.h"

class NXPrimitive;
class NXSubMeshBase
{
	friend class NXMeshResourceManager;
	friend class NXMaterialResourceManager;
public:
	NXSubMeshBase(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : 
		m_pRenderableObject(pRenderableObject), 
		m_subMeshName(subMeshName),
		m_pMaterial(nullptr), 
		m_pReplacingMaterial(nullptr), 
		m_nMatReloadingState(NXSubMeshReloadState::None)
	{}
	virtual ~NXSubMeshBase() {}

	void Update(ID3D12GraphicsCommandList* pCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pCommandList) = 0;

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) = 0;
	virtual void TryAddBuffers() = 0;

	// �Զ����㶥����������ݡ�
	// bUpdateVBIB: �Ƿ�ͬʱ����VBIB
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
	// [Warning!] ������ֱ�����ò��ʣ�
	// ��ʹ�� BindMaterial() Ϊ��������󶨲���
	virtual void SetMaterial(NXMaterial* mat) { m_pMaterial = mat; }

protected:
	std::string	m_subMeshName;

	NXRenderableObject* m_pRenderableObject;
	NXMaterial* m_pMaterial;

	AABB m_localAABB;

	NXSubMeshReloadState m_nMatReloadingState;
	std::filesystem::path m_strReplacingPath;
	NXMaterial* m_pReplacingMaterial;

	// ������Ա�ȣ���vertices indices instancedata�����ֽ���ͼ
	std::vector<NXRawMeshView>	m_rawViews;
};

template<class TVertex>
class NXSubMesh : public NXSubMeshBase
{
public:
	NXSubMesh(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMeshBase(pRenderableObject, subMeshName) {}
	virtual ~NXSubMesh() {}

	void AppendVertices(std::vector<TVertex>&& vertices)
	{
		m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
	}

	void AppendIndices(std::vector<uint32_t>&& indices)
	{
		m_indices.insert(m_indices.end(), indices.begin(), indices.end());
	}

	virtual void Render(ID3D12GraphicsCommandList* pCommandList) override
	{
		auto& subMeshViews = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_subMeshName);

		D3D12_VERTEX_BUFFER_VIEW vbv;
		if (subMeshViews.GetVBV(0, vbv))
			pCommandList->IASetVertexBuffers(0, 1, &vbv);

		D3D12_INDEX_BUFFER_VIEW ibv;
		if (subMeshViews.GetIBV(1, ibv))
			pCommandList->IASetIndexBuffer(&ibv);

		pCommandList->DrawIndexedInstanced(subMeshViews.GetIndexCount(), 1, 0, 0, 0);
	}

	virtual bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) { return false; }
	virtual void CalcLocalAABB() {}

	virtual void CalculateTangents(bool bUpdateVBIB = false) {}
	virtual void TryAddBuffers() override
	{
		auto rawBytes = std::as_bytes(std::span(m_vertices));
		m_rawViews.push_back(NXRawMeshView(rawBytes, sizeof(TVertex), NXMeshViewType::VERTEX));
		rawBytes = std::as_bytes(std::span(m_indices));
		m_rawViews.push_back(NXRawMeshView(rawBytes, sizeof(uint32_t), NXMeshViewType::INDEX));
		NXSubMeshGeometryEditor::GetInstance()->CreateBuffers(m_rawViews, m_subMeshName);
	}

	UINT GetIndexCount()	{ return (UINT)m_indices.size(); }
	UINT GetVertexCount()	{ return (UINT)m_vertices.size(); }

protected:
	std::vector<TVertex>	m_vertices;
	std::vector<UINT>		m_indices; 
};

template<class TVertex, class TInstanceData>
class NXSubMeshInstanced : public NXSubMesh<TVertex>
{
public:
	NXSubMeshInstanced(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : NXSubMesh<TVertex>(pRenderableObject, subMeshName) {}
	virtual ~NXSubMeshInstanced() {}

	void AppendInstanceData(std::vector<TInstanceData>&& instanceData)
	{
		m_instanceData.insert(m_instanceData.end(), instanceData.begin(), instanceData.end());
	}

	virtual void TryAddBuffers() override
	{
		auto rawBytes = std::as_bytes(std::span(m_vertices));
		m_rawViews.push_back(NXRawMeshView(rawBytes, sizeof(TVertex), NXMeshViewType::VERTEX));
		rawBytes = std::as_bytes(std::span(m_instanceData));
		m_rawViews.push_back(NXRawMeshView(rawBytes, sizeof(TInstanceData), NXMeshViewType::VERTEX));
		rawBytes = std::as_bytes(std::span(m_indices));
		m_rawViews.push_back(NXRawMeshView(rawBytes, sizeof(uint32_t), NXMeshViewType::INDEX));
		NXSubMeshGeometryEditor::GetInstance()->CreateBuffers(m_rawViews, m_subMeshName);
	}

	virtual void Render(ID3D12GraphicsCommandList* pCommandList) override
	{
		auto& subMeshViews = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_subMeshName);

		D3D12_VERTEX_BUFFER_VIEW vbv[2];
		if (subMeshViews.GetVBV(0, vbv[0]) && subMeshViews.GetVBV(1, vbv[1]))
			pCommandList->IASetVertexBuffers(0, 2, vbv);

		D3D12_INDEX_BUFFER_VIEW ibv;
		if (subMeshViews.GetIBV(2, ibv))
			pCommandList->IASetIndexBuffer(&ibv);

		pCommandList->DrawIndexedInstanced(subMeshViews.GetIndexCount(), m_instanceData.size(), 0, 0, 0);
	}

	uint32_t GetInstanceCount() { return (uint32_t)m_instanceData.size(); }

protected:
	std::vector<TInstanceData>	m_instanceData;
};

class NXSubMeshStandard : public NXSubMesh<VertexPNTT>
{
public:
	NXSubMeshStandard(NXRenderableObject* pRenderableObject, const std::string& subMeshName) : 
		NXSubMesh<VertexPNTT>(pRenderableObject, subMeshName) 
	{
	}
	virtual ~NXSubMeshStandard() {}

	virtual bool IsSubMeshStandard()	 override { return true; }

	void CalculateTangents(bool bUpdateVBIB = false) override;
	bool RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;
	void CalcLocalAABB() override;
};

class NXSubMeshTerrain : public NXSubMeshInstanced<VertexPNTC, InstanceData>
{
public:
	NXSubMeshTerrain(NXRenderableObject* pRenderableObject, const std::string& subMeshName) :
		NXSubMeshInstanced<VertexPNTC, InstanceData>(pRenderableObject, subMeshName) 
	{
	}

	virtual ~NXSubMeshTerrain() {}

	virtual bool IsSubMeshTerrain()		 override { return true; }

	// [Warning!] ������ֱ�����ò��ʣ�
	// ��ʹ�� BindMaterial() Ϊ��������󶨲���
	virtual void SetMaterial(NXMaterial* mat) override;

	virtual void Render(ID3D12GraphicsCommandList* pCommandList) override;

private:
	ComPtr<ID3D12CommandSignature> m_pCmdSignature;
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
	void CalcLocalAABB() override;

	void Update(ID3D12GraphicsCommandList* pCmdList, bool isHighLight);

private:
	EditorObjectID m_editorObjID;
	
	ConstantBufferVector4 m_cbDataParams;
	NXConstantBuffer<ConstantBufferVector4>	m_cbParams;
};
