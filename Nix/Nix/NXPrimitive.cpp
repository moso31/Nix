#include "NXPrimitive.h"
#include "Global.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXPrimitive::NXPrimitive()
{
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

void NXPrimitive::CalculateTangents(bool bUpdateVBIB)
{
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		GetSubMesh(i)->CalculateTangents(bUpdateVBIB);
	}
}

bool NXPrimitive::RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	// 本方法用于求Primitive和射线worldRay的交点。遍历所有三角形寻找最近交点。
	// 还可以进一步优化成BVH，但暂时没做。
	Ray localRay = worldRay.Transform(m_worldMatrixInv);
	bool bSuccess = false;

	NXHit hitInfo;
	NXHit localHitInfo;
	float localDistance = FLT_MAX;
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		auto pSubMesh = GetSubMesh(i);
		if (pSubMesh->RayCastLocal(localRay, localHitInfo, localDistance))
		{
			hitInfo = localHitInfo;
			hitInfo.LocalToWorld();

			float dist = Vector3::Distance(worldRay.position, hitInfo.position);
			if (outDist > dist)
			{
				outDist = dist;
				outHitInfo = hitInfo;

				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

void NXPrimitive::ClearSubMeshes()
{
	for (int i = 0; i < m_pSubMeshes.size(); i++)
	{
		if (m_pSubMeshes[i]) m_pSubMeshes[i].reset();
	}
}

void NXPrimitive::AddSubMesh(NXSubMeshBase* pSubMesh)
{
	auto p = std::shared_ptr<NXSubMeshBase>(pSubMesh);
	m_pSubMeshes.push_back(p);
}

void NXPrimitive::ResizeSubMesh(UINT size)
{
	m_pSubMeshes.resize(size);
}

void NXPrimitive::ReloadSubMesh(UINT index, NXSubMeshBase* pSubMesh)
{
	assert(index >= 0 && index < m_pSubMeshes.size());
	m_pSubMeshes[index].reset(pSubMesh);
}

void NXPrimitive::InitAABB()
{
	for (UINT i = 0; i < GetSubMeshCount(); i++)
	{
		auto pSubMesh = GetSubMesh(i);
		pSubMesh->CalcLocalAABB();
		
		AABB::CreateMerged(m_localAABB, m_localAABB, pSubMesh->GetLocalAABB());
	}

	NXRenderableObject::InitAABB();
}
