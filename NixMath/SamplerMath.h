#pragma once
#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DirectX
{

namespace SamplerMath
{
	Vector3 UniformSampleHemisphere(const Vector2& u, const float radius = 1.0f);
	float UniformSampleHemispherePdf();
	Vector3 UniformSampleSphere(const Vector2& u);
	float UniformSampleSpherePdf();
	Vector2 UniformSampleDisk(const Vector2& u);
	Vector3 CosineSampleHemisphere(const Vector2& u);
	float CosineSampleHemispherePdf(const float cosTheta);
	Vector2 UniformTriangleSample(const Vector2& u);
	float PowerHeuristicWeightPdf(int nf, float fPdf, int ng, float gPdf);

	float EpanechnikovKernel(const float t);

	float CubeMapAreaToOrigin(float x, float y);
	float CubeMapSolidAngleOfPixel(size_t x, size_t y, size_t texSize);
} // namespace SamplerMath

} // namespace DirectX