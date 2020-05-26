#include "NXPBRLight.h"
#include "NXScene.h"

bool NXVisibleTest::Do(const Vector3& startPosition, const Vector3& targetPosition)
{
	Vector3 visibleTestDir = targetPosition - startPosition;
	Ray ray(startPosition, visibleTestDir);
	ray.position += ray.direction * 0.001f;
	NXHit ignore;
	return !m_pScene->RayCast(ray, ignore, visibleTestDir.Length());
}

Vector3 NXPBRPointLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	// 光线发射方向
	out_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / out_wi.LengthSquared();
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, Position))
	{
		return Vector3(0.0f);
	}
	return lightRadiance;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& Direction, const Vector3& Intensity, const shared_ptr<NXScene>& pScene) : 
	Direction(Direction), 
	Intensity(Intensity),
	SceneBoundingSphere(pScene->GetBoundingSphere())
{
}

Vector3 NXPBRDistantLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	out_wi = -Direction;
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position - Direction))
	{
		return Vector3(0.0f);
	}
	return Intensity;
}
