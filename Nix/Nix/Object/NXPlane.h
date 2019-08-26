#pragma once
#include "NXPrimitive.h"

class NXPlane : public NXPrimitive
{
public:
	NXPlane();

	void Init(float width = 0.5f, float height = 0.5f);
	void Render();
	void Release();

private:
	float m_width;
	float m_height;
};
