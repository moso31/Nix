#pragma once
#include "NXInstance.h"
#include "NXIntersection.h"

class NXVisibleTest : public NXInstance<NXVisibleTest>
{
public:
	NXVisibleTest() {}
	~NXVisibleTest() {}

	void SetScene(shared_ptr<NXScene>& pScene) { m_pScene = pScene; }
	bool Do(const Vector3& startPosition, const Vector3& targetPosition);

private:
	shared_ptr<NXScene> m_pScene;
};

class NXPBRLight
{
public:
	NXPBRLight() {}
	~NXPBRLight() {}

	virtual Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) = 0;

};

// 临时PBR光源
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	// 计算点光源的入射辐射率
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