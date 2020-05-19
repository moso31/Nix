#pragma once
#include "NXPrimitive.h"

class NXSphere : public NXPrimitive
{
public:
	NXSphere();
	~NXSphere() {}

	void Init(float radius, int segmentHorizontal, int segmentVertical);

	virtual bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist) override;

private:
	float m_radius;
	UINT m_segmentVertical;
	UINT m_segmentHorizontal;
};
