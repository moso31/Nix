#include "SamplerMath.h"

Vector3 SamplerMath::UniformSampleHemisphere(const Vector2& u)
{
	// u = ����[0, 1)֮������ֵ������u�ᱻӳ�䵽��������档
	float z = u.x;
	float phi = 2 * PI * u.y;
	float r = 1.0f - sqrtf(z * z);
	float x = cosf(phi) * r;
	float y = sinf(phi) * r;
	return Vector3(x, y, z);
}

Vector3 SamplerMath::UniformSampleSphere(const Vector2& u)
{
	// u = ����[0, 1)֮������ֵ������u�ᱻӳ�䵽������档
	float z = 1.0f - 2.0f * u.x;
	float phi = 2 * PI * u.y;
	float r = 1.0f - sqrtf(z * z);
	float x = cosf(phi) * r;
	float y = sinf(phi) * r;
	return Vector3(x, y, z);
}
