#pragma once
#include "NXIntersection.h"

class NXPBRLight
{
public:
	NXPBRLight() {}
	~NXPBRLight() {}
private:

};

// ��ʱPBR��Դ
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	// ������Դ�����������
	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi);

public:
	Vector3 Position;
	Vector3 Intensity;
};