#pragma once
#include "NXRenderableObject.h"

class NXPrefab : public NXRenderableObject
{
public:
	NXPrefab();
	~NXPrefab() {}

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	void InitAABB();

protected:
};