#pragma once
#include "NXIntersection.h"

class NXPBRLight
{
public:
	NXPBRLight() {}
	~NXPBRLight() {}

	virtual Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) = 0;

};

// ��ʱPBR��Դ
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	// ������Դ�����������
	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) override;

public:
	Vector3 Position;
	Vector3 Intensity;
};

class NXPBRDistantLight : public NXPBRLight
{
public:
	NXPBRDistantLight(const Vector3& Direction, const Vector3& Intensity, const shared_ptr<NXScene>& pScene);

	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) override;

public:
	Vector3 Direction;
	Vector3 Intensity;
	BoundingSphere SceneBoundingSphere;
};