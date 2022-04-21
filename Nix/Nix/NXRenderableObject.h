#pragma once
#include "NXTransform.h"

class NXHit;

class NXRenderableObject : public NXTransform
{
public:
	NXRenderableObject();
	~NXRenderableObject() {}

	virtual void Release();

	virtual AABB GetAABBWorld();
	virtual AABB GetAABBLocal();

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist) = 0;

	virtual void InitAABB() = 0;

protected:
	AABB m_aabb;
};