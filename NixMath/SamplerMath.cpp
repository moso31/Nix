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

Vector2 SamplerMath::UniformSampleDisk(const Vector2& u)
{
	float r = sqrtf(u[0]);
	float phi = XM_2PI * u[1];
	float x = cosf(phi) * r;
	float y = sinf(phi) * r;
	return Vector2(x, y);
}

Vector3 SamplerMath::CosineSampleHemisphere(const Vector2& u)
{
	Vector2 v = UniformSampleDisk(u);
	float z = sqrtf(1.0f - v.x * v.x - v.y * v.y);
	return Vector3(v.x, v.y, z);
}
