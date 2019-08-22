#pragma once
#include "NXPrimitive.h"

class NXCylinder : public NXPrimitive
{
public:
	NXCylinder() = default;

	void Init(float radius, float length, int segmentCircle, int segmentLength);
	void Update();
	void Render();
	void Release();

private:
};
