#include "SamplerMath.h"

Vector3 SamplerMath::UniformSampleHemisphere(const Vector2& u)
{
	// u = 两个[0, 1)之间的随机值。最终u会被映射到半球体表面。
	float z = u.x;
	float phi = XM_2PI * u.y;
	float r = 1.0f - sqrtf(z * z);
	float x = cosf(phi) * r;
	float y = sinf(phi) * r;
	return Vector3(x, y, z);
}

float DirectX::SamplerMath::UniformSampleHemispherePdf()
{
	return XM_1DIV2PI;	// 1 / (4 * pi * r * r * 0.5), where r = 1, 0.5 means "half" sphere.
}

Vector3 SamplerMath::UniformSampleSphere(const Vector2& u)
{
	// u = 两个[0, 1)之间的随机值。最终u会被映射到球体表面。
	float z = 1.0f - 2.0f * u.x;
	float phi = XM_2PI * u.y;
	float r = 1.0f - sqrtf(z * z);
	float x = cosf(phi) * r;
	float y = sinf(phi) * r;
	return Vector3(x, y, z);
}

float DirectX::SamplerMath::UniformSampleSpherePdf()
{
	return XM_1DIV4PI;	// 1 / (4 * pi * r * r), where r = 1.
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

float DirectX::SamplerMath::CosineSampleHemispherePdf(const float cosTheta)
{
	return cosTheta * XM_1DIVPI;
}

Vector2 SamplerMath::UniformTriangleSample(const Vector2& u)
{
	float sx = sqrtf(u.x);
	float x = 1.0f - sx;
	float y = sx * u.y;
	return Vector2(x, y);
}

float SamplerMath::PowerHeuristicWeightPdf(int nf, float fPdf, int ng, float gPdf)
{
	// 使用启发式方案对进行pdf采样的权重计算。
	float f = nf * fPdf, g = ng * gPdf;
	return (f * f) / (f * f + g * g);
}