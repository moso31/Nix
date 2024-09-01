#include "NXPrimitive.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXAllocatorManager.h"
#include "NXCamera.h"
#include "NXTimer.h"

NXPrimitive::NXPrimitive()
{
	InitCBData();
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

void NXPrimitive::InitCBData()
{
}

void NXPrimitive::Update(ID3D12GraphicsCommandList* pCmdList)
{
	auto* pCamera = NXResourceManager::GetInstance()->GetCameraManager()->GetCamera("Main Camera");
	auto& mxView = pCamera->GetViewMatrix();
	auto& mxWorld = m_worldMatrix;
	auto& mxWorldView = mxWorld * mxView;

	m_cbDataObject.world = mxWorld.Transpose();
	m_cbDataObject.worldInverseTranspose = mxWorld.Invert(); // it actually = m_worldMatrix.Invert().Transpose().Transpose();
	m_cbDataObject.worldView = mxWorldView.Transpose();
	m_cbDataObject.worldViewInverseTranspose = (mxWorldView).Invert();
	m_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();

	m_cbObject.Update(m_cbDataObject);

	pCmdList->SetGraphicsRootConstantBufferView(0, m_cbObject.CurrentGPUAddress());
}
