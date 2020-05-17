#include "NXPBRLight.h"

Vector3 NXPBRPointLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi)
{
	// ���߷��䷽��
	out_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / out_wi.LengthSquared();
	out_wi.Normalize();

	// ��ʱ����Visibility tester

	return lightRadiance;
}
