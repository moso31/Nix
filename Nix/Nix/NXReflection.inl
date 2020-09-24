#pragma once
#include "NXReflection.h"

inline Vector3 Reflect(const Vector3& in, const Vector3& normal)
{
	return 2.0f * in.Dot(normal) * normal - in;
}

inline bool Refract(const Vector3& in, const Vector3& normal, float etaI, float etaT, Vector3& outRefract)
{
	float eta = etaI / etaT;
	float cosThetaI = in.Dot(normal);
	float sin2ThetaT = eta * eta * (1 - cosThetaI * cosThetaI);
	if (sin2ThetaT >= 1.0f)
	{
		// 折射角超过90度，形成全内反射。此时折射无作用。
		outRefract = Vector3(0.0f);
		return false;
	}
	float cosThetaT = sqrtf(1.0f - sin2ThetaT);
	outRefract = (eta * cosThetaI - cosThetaT) * normal - eta * in;
	return true;
}

inline float CosTheta(const Vector3& w)
{
	return w.z;
}

inline float AbsCosTheta(const Vector3& w)
{
	return fabsf(w.z);
}

inline float Cos2Theta(const Vector3& w)
{
	return w.z * w.z;
}

inline float Sin2Theta(const Vector3& w)
{
	return fmaxf(0.0f, 1.0f - Cos2Theta(w));
}

inline float SinTheta(const Vector3& w)
{
	return sqrtf(Sin2Theta(w));
}

inline float Tan2Theta(const Vector3& w)
{
	return Sin2Theta(w) / Cos2Theta(w);
}

inline float TanTheta(const Vector3& w)
{
	return sqrtf(Tan2Theta(w));
}

inline bool IsSameHemisphere(const Vector3& wo, const Vector3& wi)
{
	return wo.z * wi.z > 0;
}
