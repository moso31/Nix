#pragma once
#include "NXPrimitive.h"

class NXCylinder : public NXPrimitive
{
public:
	NXCylinder();
	~NXCylinder() {}

	void Init(float radius, float length, int segmentCircle, int segmentLength);
	void Render();
	void Release();

private:
	float m_radius;
	float m_length;
	float m_segmentCircle;
	float m_segmentLength;
};
