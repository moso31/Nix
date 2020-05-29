#pragma once
#include "NXInstance.h"
#include "NXIntersection.h"
#include "NXPrimitive.h"

class NXVisibleTest : public NXInstance<NXVisibleTest>
{
public:
	NXVisibleTest() {}
	~NXVisibleTest() {}

	void SetScene(shared_ptr<NXScene>& pScene) { m_pScene = pScene; }
	bool Do(const Vector3& startPosition, const Vector3& targetPosition);

protected:
	shared_ptr<NXScene> m_pScene;
};

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
	NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, const shared_ptr<NXScene>& pScene);

	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) override;

public:
	Vector3 Direction;
	Vector3 Radiance;
	BoundingSphere SceneBoundingSphere;
};

class NXPBRAreaLight : public NXPBRLight
{
public:
	NXPBRAreaLight(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& Radiance);

	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) override;

	// �����samplePosition��targetDirection��������ߵ�Radianceֵ��
	// �ṩ���淢������surfaceNormal�����жϱ��ν����Ƿ�����
	Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection);

public:
	Vector3 Radiance;

private:
	shared_ptr<NXPrimitive> m_pPrimitive;
};

class NXPBREnvironmentLight : public NXPBRLight
{
public:
	NXPBREnvironmentLight(const shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Intensity, float SceneRadius);

	Vector3 SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf) override;
	Vector3 GetRadiance(const Vector3& targetDirection);

public:
	Vector3 Intensity;
	float SceneRadius;

private:
	shared_ptr<NXCubeMap> m_pCubeMap;
};