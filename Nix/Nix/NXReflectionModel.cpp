#include "NXReflectionModel.h"
#include "SamplerMath.h"
#include "NXRandom.h"

bool Refract(const Vector3& dirIn, const Vector3& dirNormal, float etaI, float etaT, Vector3& outRefract)
{
	float eta = etaI / etaT;
	float cosThetaI = dirIn.Dot(dirNormal);
	float sin2ThetaT = eta * eta * (1 - cosThetaI * cosThetaI);
	if (sin2ThetaT >= 1.0f)
	{
		// 折射角超过90度，形成全内反射。此时折射无作用。
		outRefract = Vector3(0.0f);
		return false;
	}
	float cosThetaT = sqrtf(1.0f - sin2ThetaT);
	outRefract = (eta * cosThetaI - cosThetaT) * dirNormal - eta * dirIn;
	return true;
}

Vector3 NXRLambertianReflect::f(const Vector3& wo, const Vector3& wi)
{
	return R / XM_PI;
}

Vector3 NXRLambertianReflect::Sample_f(const Vector3& wo, Vector3& wi)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	wi = SamplerMath::UniformSampleHemisphere(u);
	// （反射空间）强制约束wi，使其和wo始终处于同一半球。
	if (wo.z < 0.0f) wi.z *= -1;
	// pdf skip.
	return f(wo, wi);
}

Vector3 NXRPrefectReflect::Sample_f(const Vector3& wo, Vector3& wi)
{
	// （反射空间）Reflect方法的优化。
	wi = Vector3(-wo.x, -wo.y, wo.z);
	// pdf skip.
	float cosThetaI = abs(wi.z);
	return Vector3(Fr(wi) * R / cosThetaI);
}

Vector3 NXRPrefectTransmission::Sample_f(const Vector3& wo, Vector3& wi)
{
	// 确定etaA和etaB的入射/折射关系
	bool entering = wo.z > 0;	// 是从外部进入到内部吗？
	float etaI = entering ? etaA : etaB;
	float etaT = entering ? etaB : etaA;

	// 如果wo朝向和法线相反，那么就应该反转etaI和etaT。
	Vector3 normal = Vector3(0.0f, 0.0f, entering ? 1.0f : -1.0f);
	if (!Refract(wo, normal, etaI, etaT, wi))
		return;
	// pdf skip.

	Vector3 f_transmittion = (1.0f - Fr(wi)) / abs(wi.z) * T;
	// 根据PBRT指示，在计算基于Camera出发的射线的时候，不需要etaT^2/etaI^2。
	// 16章有解释，但现在还没看到那里，先维持现状。
	// 还有，公式8.8给的是etaT^2/etaI^2，但PBRT实际代码是etaI^2/etaT^2，反过来了。
	// 具体原因也不明。待查。
	// if (!Camera) f_transmittion *= (etaT * etaT) / (etaI * etaI);
	return f_transmittion;
}
