#pragma once
#include "NXPrimitive.h"

enum NXPlaneAxis
{
	POSITIVE_X,
	POSITIVE_Y,
	POSITIVE_Z,
	NEGATIVE_X,
	NEGATIVE_Y,
	NEGATIVE_Z,
};

class NXPlane : public NXPrimitive
{
public:
	NXPlane();
	~NXPlane() {}

	void Init(float width = 0.5f, float height = 0.5f, NXPlaneAxis Axis = POSITIVE_Y);

private:
	float m_width;
	float m_height;
};
