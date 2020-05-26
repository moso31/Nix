#include "NXPBRLight.h"
#include "NXScene.h"

Vector3 NXPBRPointLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	// ���߷��䷽��
	out_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / out_wi.LengthSquared();
	out_wi.Normalize();
	out_pdf = 1;
	// ��ʱ����Visibility tester
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
	// ��ʱ����Visibility tester
	return Intensity;
}
