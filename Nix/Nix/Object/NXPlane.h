#pragma once
#include "NXPrimitive.h"

class NXPlane : public NXPrimitive
{
public:
	NXPlane() = default;

	void Init(float width = 0.5f, float height = 0.5f);
	void Update();
	void Render();
	void Release();

private:
};
