#pragma once
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace NXReflection
{
	Vector3 Reflect(const Vector3& in, const Vector3& normal);
	bool Refract(const Vector3& in, const Vector3& normal, float etaI, float etaT, Vector3& outRefract);

	float CosTheta(const Vector3& w);
	float AbsCosTheta(const Vector3& w);
	float Cos2Theta(const Vector3& w);
	float Sin2Theta(const Vector3& w);
	float SinTheta(const Vector3& w);
	float Tan2Theta(const Vector3& w);
	float TanTheta(const Vector3& w);

	bool IsSameHemisphere(const Vector3& wo, const Vector3& wi);

#include "NXReflection.inl"
}