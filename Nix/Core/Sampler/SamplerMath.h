#pragma once
#include "SimpleMath.inl"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DirectX
{

namespace SamplerMath
{
	Vector3 UniformSampleHemisphere(const Vector2& u);
	Vector3 UniformSampleSphere(const Vector2& u);
} // namespace SamplerMath

} // namespace DirectX