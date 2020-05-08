#pragma once
#include "NXPrimitive.h"

class NXSphere : public NXPrimitive
{
public:
	NXSphere();
	~NXSphere() {}

	void Init(float radius, int segmentHorizontal, int segmentVertical);

private:
	float m_radius;
	UINT m_segmentVertical;
	UINT m_segmentHorizontal;
};
