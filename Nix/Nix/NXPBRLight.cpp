#include "NXPBRLight.h"
#include "NXScene.h"

bool NXVisibleTest::Do(const Vector3& startPosition, const Vector3& targetPosition)
{
	Vector3 visibleTestDir = targetPosition - startPosition;
	float maxDist = visibleTestDir.Length();
	visibleTestDir.Normalize();
	Ray ray(startPosition, visibleTestDir);
	ray.position += ray.direction * 0.001f;
	NXHit ignore;
	return !m_pScene->RayCast(ray, ignore, maxDist);
}

Vector3 NXPBRPointLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	// 光线发射方向
	out_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / out_wi.LengthSquared();
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, Position))
		return Vector3(0.0f);

	return lightRadiance;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, const shared_ptr<NXScene>& pScene) : 
	Direction(Direction), 
	Radiance(Radiance),
	SceneBoundingSphere(pScene->GetBoundingSphere())
{
}

Vector3 NXPBRDistantLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	out_wi = -Direction;
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position - Direction))
		return Vector3(0.0f);

	return Radiance;
}

NXPBRAreaLight::NXPBRAreaLight(const Vector3& Radiance, const shared_ptr<NXPrimitive>& pPrimitive) :
	Radiance(Radiance),
	m_pPrimitive(pPrimitive)
{
}

Vector3 NXPBRAreaLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	Vector3 samplePoint, lightSurfaceNormal;
	m_pPrimitive->SampleFromSurface(samplePoint, lightSurfaceNormal, out_pdf);
	out_wi = samplePoint - hitInfo.position;
	out_wi.Normalize();

	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, samplePoint))
		return Vector3(0.0f);	// 无效光源：被场景中其他物体挡住

	return GetRadiance(samplePoint, lightSurfaceNormal, -out_wi);
}

Vector3 NXPBRAreaLight::GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection)
{
	if (lightSurfaceNormal.Dot(targetDirection) <= 0)
		return Vector3(0.0f);	// 无效光源：采样射线方向背朝表面
	return Radiance;
}
