#include "NXPrimitive.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPBRMaterial.h"

NXPrimitive::NXPrimitive()
{
	m_type = NXType::ePrimitive;
}

void NXPrimitive::UpdateViewParams()
{
	// ����ΪʲôҪת�ã�������û����һ��ת�á�
	// ��ͷ��һ�°ɣ���������������������ʲô�ο�ϵ�ˡ���
	auto mxWorld = m_worldMatrix.Transpose();
	NXGlobalBufferManager::m_cbDataObject.world = mxWorld;
	NXGlobalBufferManager::m_cbDataObject.worldInverseTranspose = m_worldMatrix.Invert(); // it actually = m_worldMatrix.Invert().Transpose().Transpose();

	auto viewMatrix = NXGlobalBufferManager::m_cbDataObject.view.Transpose();
	NXGlobalBufferManager::m_cbDataObject.worldViewInverseTranspose = (m_worldMatrix * viewMatrix).Invert();

	auto mxWorldView = (m_worldMatrix * viewMatrix).Transpose();
	NXGlobalBufferManager::m_cbDataObject.worldView = mxWorldView;

	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);
}

void NXPrimitive::Release()
{
	NXObject::Release();
}

void NXPrimitive::CalculateTangents(bool bUpdateVertexIndexBuffer)
{
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		GetSubMesh(i)->CalculateTangents(bUpdateVertexIndexBuffer);
	}
}

AABB NXPrimitive::GetAABBWorld()
{
	AABB worldAABB;
	AABB::Transform(m_aabb, m_worldMatrix, worldAABB);
	return worldAABB;
}

AABB NXPrimitive::GetAABBLocal() const
{
	return m_aabb;
}

bool NXPrimitive::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	// ��������������Ѱ��������㡣�����Խ�һ���Ż���BVH������ʱû����
	Ray localRay = worldRay.Transform(m_worldMatrixInv);
	bool bSuccess = false;

	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		auto pSubMesh = GetSubMesh(i);
		for (UINT j = 0; j < pSubMesh->GetFaceCount(); j++)
		{
			NXTriangle face = pSubMesh->GetFaceTriangle(j);
			if (face.RayCast(localRay, outHitInfo, outDist))
			{
				outHitInfo.faceIndex = i;
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

UINT NXPrimitive::GetFaceCount()
{
	UINT result = 0;
	for (UINT i = 0; i < GetSubMeshCount(); i++) 
		result += GetSubMesh(i)->GetFaceCount();
	return result;
}

void NXPrimitive::ClearSubMeshes()
{
	for (int i = 0; i < m_pSubMeshes.size(); i++)
	{
		if (m_pSubMeshes[i]) m_pSubMeshes[i].reset();
	}
}

void NXPrimitive::AddSubMesh(NXSubMesh* pSubMesh)
{
	auto p = std::shared_ptr<NXSubMesh>(pSubMesh);
	m_pSubMeshes.push_back(p);
}

void NXPrimitive::ResizeSubMesh(UINT size)
{
	m_pSubMeshes.resize(size);
}

void NXPrimitive::ReloadSubMesh(UINT index, NXSubMesh* pSubMesh)
{
	assert(index >= 0 && index < m_pSubMeshes.size());
	m_pSubMeshes[index].reset(pSubMesh);
}

void NXPrimitive::InitAABB()
{
	m_points.clear();
	m_points.reserve(GetFaceCount());
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		auto pSubMesh = GetSubMesh(i);
		const VertexPNTT* pVertexData = pSubMesh->GetVertexData();

		for (UINT j = 0; j < pSubMesh->GetVertexCount(); j++)
		{
			m_points.push_back(pVertexData[j].pos);
		}
	}
	AABB::CreateFromPoints(m_aabb, m_points.size(), m_points.data(), sizeof(Vector3));
}
