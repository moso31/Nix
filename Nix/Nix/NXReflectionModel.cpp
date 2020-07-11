#include "NXReflectionModel.h"
#include "SamplerMath.h"
#include "NXRandom.h"

using namespace NXReflection;

Vector3 NXReflection::Reflect(const Vector3& dirIn, const Vector3& dirNormal)
{
	return 2.0f * dirIn.Dot(dirNormal) * dirNormal - dirIn;
}

bool NXReflection::Refract(const Vector3& dirIn, const Vector3& dirNormal, float etaI, float etaT, Vector3& outRefract)
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

float NXReflection::CosTheta(const Vector3& w)
{
	return w.z;
}

float NXReflection::AbsCosTheta(const Vector3& w)
{
	return fabsf(w.z);
}

float NXReflection::Cos2Theta(const Vector3& w)
{
	return w.z * w.z;
}

float NXReflection::Sin2Theta(const Vector3& w)
{
	return 1.0f - Cos2Theta(w);
}

float NXReflection::SinTheta(const Vector3& w)
{
	return sqrtf(Sin2Theta(w));
}

float NXReflection::Tan2Theta(const Vector3& w)
{
	return Sin2Theta(w) / Cos2Theta(w);
}

float NXReflection::TanTheta(const Vector3& w)
{
	return sqrtf(Tan2Theta(w));
}

bool NXReflection::IsSameHemisphere(const Vector3& wo, const Vector3& wi)
{
	return wo.z * wi.z > 0;
}

float NXReflectionModel::Pdf(const Vector3& wo, const Vector3& wi)
{
	if (IsSameHemisphere(wo, wi)) 
		// 默认使用余弦采样，所以pdf(wi) = |wi·n|/Pi
		// 绝对值是为了确保wi处于n朝向的半球。
		return AbsCosTheta(wi) * XM_1DIVPI; 
	return 0.0f;	// 不在同一半球没有被采样的必要，pdf直接返回0
}

bool NXReflectionModel::IsMatchingType(ReflectionType type)
{
	return (m_type & type) == m_type;
}

Vector3 NXReflectionModel::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	wi = SamplerMath::CosineSampleHemisphere(u);
	// （反射空间）强制约束wi，使其和wo始终处于同一半球。
	if (wo.z < 0.0f) wi.z *= -1;
	pdf = Pdf(wo, wi);
	return f(wo, wi);
}

Vector3 NXRLambertianReflection::f(const Vector3& wo, const Vector3& wi)
{
	return R / XM_PI;
}

Vector3 NXRPrefectReflection::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	// （反射空间）Reflect方法的优化。
	wi = Vector3(-wo.x, -wo.y, wo.z);
	float cosThetaI = CosTheta(wi);
	pdf = 1.0f;	// 完美反射模型被选中时pdf=1，未被选中时pdf=0
	return Vector3(fresnel->FresnelReflectance(cosThetaI) * R / abs(cosThetaI));
}

Vector3 NXRPrefectTransmission::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	bool entering = CosTheta(wo) > 0;	// 是从外部进入到内部吗？

	// 确定etaA和etaB的入射/折射关系
	// 这里使用的方向和PBRT中使用的向量反过来了：wo位于入射介质，wi位于出射介质。
	// 出现这种问题是Sample_f()统一化代码格式导致的。推导时需要注意。
	float etaI = entering ? etaA : etaB;
	float etaT = entering ? etaB : etaA;

	// 如果wo朝向和法线相反，那么就应该反转etaI和etaT。
	Vector3 normal = Vector3(0.0f, 0.0f, entering ? 1.0f : -1.0f);
	if (!Refract(wo, normal, etaI, etaT, wi))
		return Vector3(0.0f);	// 全内反射情况

	pdf = 1.0f;	// 完美反射模型被选中时pdf=1，未被选中时pdf=0
	
	Vector3 f_transmittion = T * (Vector3(1.0f) - fresnel.FresnelReflectance(CosTheta(wi))) / AbsCosTheta(wi);

	bool IsFromCamera = true;
	// 如果是从相机出发的射线，考虑折射过程中微分角压缩比。
	// 如果是从光源出发，则此项正好和伴随BSDF相互抵消，不用考虑。
	if (IsFromCamera) f_transmittion *= etaI * etaI / (etaT * etaT);
	return f_transmittion;
}

Vector3 NXRMicrofacetReflection::f(const Vector3& wo, const Vector3& wi)
{
	Vector3 wh = wo + wi;
	wh.Normalize();
	// 使用绝对值确保wo、wi均和n处于同一半球
	float cosThetaO = AbsCosTheta(wo);
	float cosThetaI = AbsCosTheta(wi);
	return R * distrib->D(wh) * distrib->G(wo, wi) * fresnel->FresnelReflectance(wi.Dot(wh)) / (4.0f * cosThetaO * cosThetaI);
}

Vector3 NXRMicrofacetReflection::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	Vector3 wh = distrib->Sample_wh(wo);
	wi = Reflect(wo, wh);
	if (!IsSameHemisphere(wo, wi)) return Vector3(0.0f);

	pdf = distrib->Pdf(wh) / (4.0f * wi.Dot(wh));
	return f(wo, wi);
}

float NXRMicrofacetReflection::Pdf(const Vector3& wo, const Vector3& wi)
{
	if (!IsSameHemisphere(wo, wi)) return 0;

	Vector3 wh = wo + wi;
	wh.Normalize();
	return distrib->Pdf(wh) / (4.0f * wi.Dot(wh));
}
