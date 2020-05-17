#pragma once
#include "NXIntersection.h"

class NXPBRLight
{
public:
	NXPBRLight() {}
	~NXPBRLight() {}
private:

};

// 临时PBR光源
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	// 计算点光源的入射辐射率
	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi);

public:
	Vector3 Position;
	Vector3 Intensity;
};