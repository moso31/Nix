#include "NXSubMesh.h"
#include "NXGlobalDefinitions.h"
#include "NXPrimitive.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXCamera.h"
#include "NXTimer.h"

template class NXSubMesh<VertexPNTT>;
template class NXSubMesh<VertexEditorObjects>;

void NXSubMeshBase::Update(ID3D12GraphicsCommandList* pCommandList)
{
	if (m_pMaterial) m_pMaterial->Update();
}

void NXSubMeshBase::MarkReplacing(const std::filesystem::path& replaceMaterialPath)
{
	if (m_nMatReloadingState == NXSubMeshReloadState::None)
	{
		m_nMatReloadingState = NXSubMeshReloadState::Start;
		NXResourceManager::GetInstance()->GetMeshManager()->AddReplacingSubMesh(this);
		m_strReplacingPath = replaceMaterialPath;
	}
}

void NXSubMeshBase::SwitchToLoadingMaterial()
{
	auto pLoadingMaterial = NXResourceManager::GetInstance()->GetMaterialManager()->GetLoadingMaterial();
	if (pLoadingMaterial)
		NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(this, pLoadingMaterial);
}

void NXSubMeshBase::SwitchToReplacingMaterial()
{
	if (m_pReplacingMaterial)
		NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(this, m_pReplacingMaterial);
}

void NXSubMeshBase::OnReplaceFinish()
{
	m_nMatReloadingState = NXSubMeshReloadState::Finish;
	NXResourceManager::GetInstance()->GetMeshManager()->AddReplacingSubMesh(this);
}

template<class TVertex>
void NXSubMesh<TVertex>::Render(ID3D12GraphicsCommandList* pCommandList)
{
	auto& subMeshViews = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_subMeshName);
	pCommandList->IASetVertexBuffers(0, 1, &subMeshViews.vbv);
	pCommandList->IASetIndexBuffer(&subMeshViews.ibv);
	pCommandList->DrawIndexedInstanced(subMeshViews.indexCount, 1, 0, 0, 0);
}

template<class TVertex>
bool NXSubMesh<TVertex>::RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	bool bSuccess = false;
	for (UINT i = 0, faceId = 0; i < m_indices.size(); i += 3, faceId++)
	{
		Triangle face(m_vertices[m_indices[i + 0]].pos, m_vertices[m_indices[i + 1]].pos, m_vertices[m_indices[i + 2]].pos);
		if (face.Intersects(localRay, outHitInfo.position, outDist))
		{
			outHitInfo.pSubMesh = this;
			outHitInfo.faceIndex = faceId;
			bSuccess = true;
		}
	}

	return bSuccess;
}

template<class TVertex>
void NXSubMesh<TVertex>::CalcLocalAABB()
{
	Vector3 vMin(+FLT_MAX);
	Vector3 vMax(-FLT_MAX);

	for (int i = 0; i < m_vertices.size(); i++)
	{
		vMin = Vector3::Min(vMin, m_vertices[i].pos);
		vMax = Vector3::Max(vMax, m_vertices[i].pos);
	}

	m_localAABB = AABB(vMin, vMax);
}

template<class TVertex>
void NXSubMesh<TVertex>::TryAddBuffers()
{
	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(m_vertices, m_indices, m_subMeshName, true);
}

void NXSubMeshStandard::CalculateTangents(bool bUpdateVBIB)
{
	int faceCount = (int)m_indices.size() / 3;
	for (int i = 0; i < m_indices.size(); i += 3)
	{
		auto& P0 = m_vertices[m_indices[i + 0]];
		auto& P1 = m_vertices[m_indices[i + 1]];
		auto& P2 = m_vertices[m_indices[i + 2]];

		auto e0 = P1.pos - P0.pos;
		auto e1 = P2.pos - P0.pos;

		auto uv0 = P1.tex - P0.tex;
		auto uv1 = P2.tex - P0.tex;

		float u0 = uv0.x;
		float v0 = uv0.y;
		float u1 = uv1.x;
		float v1 = uv1.y;

		float detInvUV = 1.0f / (u0 * v1 - v0 * u1);

		Vector3 dpdu = v1 * e0 - v0 * e1;
		dpdu.Normalize();
		P0.tangent = dpdu;
		P1.tangent = dpdu;
		P2.tangent = dpdu;
	}

	if (bUpdateVBIB)
	{
		TryAddBuffers();
	}
}

NXSubMeshEditorObjects::NXSubMeshEditorObjects(NXPrimitive* pPrimitive, const std::string& subMeshName, EditorObjectID id) : 
	NXSubMesh<VertexEditorObjects>(pPrimitive, subMeshName), 
	m_editorObjID(id) 
{
	m_cbParams.CreateFrameBuffers(NXCBufferAllocator, NXDescriptorAllocator);
}

bool NXSubMeshEditorObjects::RayCastLocal(const Ray& localRay, NXHit& outHitInfo, float& outDist)
{
	// Editor Objects 使用双面 ray-tri isect。
	bool bSuccess = false;
	for (UINT i = 0, faceId = 0; i < m_indices.size(); i += 3, faceId++)
	{
		Triangle faceFront(m_vertices[m_indices[i + 0]].pos, m_vertices[m_indices[i + 1]].pos, m_vertices[m_indices[i + 2]].pos);
		Triangle faceBack (m_vertices[m_indices[i + 0]].pos, m_vertices[m_indices[i + 2]].pos, m_vertices[m_indices[i + 1]].pos);
		if (faceFront.Intersects(localRay, outHitInfo.position, outDist) || faceBack.Intersects(localRay, outHitInfo.position, outDist))
		{
			outHitInfo.pSubMesh = this;
			outHitInfo.faceIndex = faceId;
			bSuccess = true;
		}
	}

	return bSuccess;
}

void NXSubMeshEditorObjects::Update(ID3D12GraphicsCommandList* pCmdList, bool isHighLight)
{
	// 判断当前SubMesh是否高亮显示
	m_cbParams.Get().value.x = isHighLight ? 1.0f : 0.0f;
	m_cbParams.UpdateBuffer();
	pCmdList->SetGraphicsRootConstantBufferView(2, m_cbParams.GetGPUHandle()); // b2 update.
}
