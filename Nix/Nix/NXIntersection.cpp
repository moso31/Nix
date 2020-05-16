#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXHit::NXHit()
{
}

NXHit::~NXHit()
{
}

bool NXHit::RayCast(const shared_ptr<NXScene>& pScene, const Ray& ray, NXHitInfo& oInfo)
{
	float minDist = FLT_MAX;
	auto pPrimitives = pScene->GetPrimitives();

	// 目前还是用遍历找的……将来改成KD树或BVH树。
	for (auto it = pPrimitives.begin(); it != pPrimitives.end(); it++)
	{
		Ray LocalRay(
			Vector3::Transform(ray.position, (*it)->GetWorldMatrixInv()),
			Vector3::TransformNormal(ray.direction, (*it)->GetWorldMatrixInv())
		);
		LocalRay.direction.Normalize();

		// ray-aabb
		if (LocalRay.IntersectsFast((*it)->GetAABBLocal(), oInfo.distance))
		{
			// ray-triangle
			if ((*it)->Intersect(LocalRay, oInfo.position, oInfo.distance))
			{
				if (minDist > oInfo.distance)
				{
					minDist = oInfo.distance;
					oInfo.primitive = *it;
				}
			}
		}
	}

	if (oInfo.primitive)
	{
		oInfo.position = Vector3::Transform(oInfo.position, oInfo.primitive->GetWorldMatrix());
		oInfo.distance = Vector3::Distance(ray.position, oInfo.position);
	}

	return oInfo.primitive != nullptr;
}
