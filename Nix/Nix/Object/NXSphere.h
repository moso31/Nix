#pragma once
#include "NXPrimitive.h"

class NXSphere : public NXPrimitive
{
public:
	NXSphere() = default;

	void Init(float radius, int segmentHorizontal, int segmentVertical);
	void Render();
	void Release();

private:
	float m_radius;
	UINT m_segmentVertical;
	UINT m_segmentHorizontal;
};
