#include "NXPrimitive.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXPrimitive::NXPrimitive()
{
	m_type = NXType::ePrimitive;
}

void NXPrimitive::UpdateViewParams()
{
	auto mxWorld = m_worldMatrix.Transpose();
	NXGlobalBufferManager::m_cbDataObject.world = mxWorld;
	NXGlobalBufferManager::m_cbDataObject.worldInverseTranspose = m_worldMatrix.Invert(); // it actually = m_worldMatrix.Invert().Transpose().Transpose();

	auto mxView = NXGlobalBufferManager::m_cbDataObject.view.Transpose();
	NXGlobalBufferManager::m_cbDataObject.worldViewInverseTranspose = (m_worldMatrix * mxView).Invert();

	auto mxWorldView = (m_worldMatrix * mxView).Transpose();
	NXGlobalBufferManager::m_cbDataObject.worldView = mxWorldView;

	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);
}

void NXPrimitive::CalculateTangents(bool bUpdateVertexIndexBuffer)
{
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		GetSubMesh(i)->CalculateTangents(bUpdateVertexIndexBuffer);
	}
}

bool NXPrimitive::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	// 本方法用于求Primitive和射线worldRay的交点。遍历所有三角形寻找最近交点。
	// 还可以进一步优化成BVH，但暂时没做。
	Ray localRay = worldRay.Transform(m_worldMatrixInv);
	bool bSuccess = false;

	float dist = outDist;
	NXHit hitInfo;

	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		auto pSubMesh = GetSubMesh(i);
		if (pSubMesh->RayCastLocal(localRay, hitInfo, dist) && dist < outDist)
		{
			outHitInfo = hitInfo;
			outDist = dist;
			bSuccess = true;
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
