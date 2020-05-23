#pragma once
#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DirectX
{

namespace SamplerMath
{
	Vector3 UniformSampleHemisphere(const Vector2& u);
	Vector3 UniformSampleSphere(const Vector2& u);
	Vector2 UniformSampleDisk(const Vector2& u);
	Vector3 CosineSampleHemisphere(const Vector2& u);
} // namespace SamplerMath

} // namespace DirectX