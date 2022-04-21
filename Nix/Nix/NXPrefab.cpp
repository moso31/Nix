#include "NXPrefab.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"

NXPrefab::NXPrefab()
{
	m_type = NXType::ePrefab;
}

bool NXPrefab::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	// ������������Primitive������worldRay�Ľ��㡣��������������Ѱ��������㡣
	// �����Խ�һ���Ż���BVH������ʱû����
	Ray localRay = worldRay.Transform(m_worldMatrixInv);
	bool bSuccess = false;

	float dist = outDist;
	NXHit hitInfo;

	for (auto pChild : GetChilds())
	{
		if (pChild->GetType() == NXType::ePrimitive)
		{
			NXPrimitive* pPrim = static_cast<NXPrimitive*>(pChild);
			if (pPrim->RayCast(worldRay, outHitInfo, dist) && dist < outDist)
			{
				outHitInfo = hitInfo;
				outDist = dist;
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

void NXPrefab::InitAABB()
{
	for (auto pChild : GetChilds())
	{
		if (pChild->GetType() == NXType::ePrimitive)
		{
			auto* pPrim = static_cast<NXPrimitive*>(pChild);
			pPrim->InitAABB();
		}
	}
}
