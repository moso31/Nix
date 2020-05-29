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
		// ����ǳ���90�ȣ��γ�ȫ�ڷ��䡣��ʱ���������á�
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
		// Ĭ��ʹ�����Ҳ���������pdf(wi) = |wi��n|/Pi
		// ����ֵ��Ϊ��ȷ��wi����n����İ���
		return AbsCosTheta(wi) * XM_1DIVPI; 
	return 0.0f;	// ����ͬһ����û�б������ı�Ҫ��pdfֱ�ӷ���0
}

Vector3 NXReflectionModel::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	wi = SamplerMath::CosineSampleHemisphere(u);
	// ������ռ䣩ǿ��Լ��wi��ʹ���woʼ�մ���ͬһ����
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
	// ������ռ䣩Reflect�������Ż���
	wi = Vector3(-wo.x, -wo.y, wo.z);
	float cosThetaI = CosTheta(wi);
	pdf = 1.0f;	// ��������ģ�ͱ�ѡ��ʱpdf=1��δ��ѡ��ʱpdf=0
	return Vector3(fresnel->FresnelReflectance(cosThetaI) * R / abs(cosThetaI));
}

Vector3 NXRPrefectTransmission::Sample_f(const Vector3& wo, Vector3& wi, float& pdf)
{
	// ȷ��etaA��etaB������/�����ϵ
	bool entering = CosTheta(wo) > 0;	// �Ǵ��ⲿ���뵽�ڲ����ǵĻ��ͷ�ת����/������ʡ�
	float etaI = entering ? etaA : etaB;
	float etaT = entering ? etaB : etaA;

	// ���wo����ͷ����෴����ô��Ӧ�÷�תetaI��etaT��
	Vector3 normal = Vector3(0.0f, 0.0f, entering ? 1.0f : -1.0f);
	if (!Refract(wo, normal, etaI, etaT, wi))
		return Vector3(0.0f);	// ȫ�ڷ������

	pdf = 1.0f;	// ��������ģ�ͱ�ѡ��ʱpdf=1��δ��ѡ��ʱpdf=0
	
	Vector3 f_transmittion = T * (Vector3(1.0f) - fresnel.FresnelReflectance(CosTheta(wi))) / AbsCosTheta(wi);
	// ����PBRTָʾ���ڼ������Camera���������ߵ�ʱ�򣬲���ҪetaT^2/etaI^2��
	// 16���н��ͣ������ڻ�û���������ά����״��
	// ���У���ʽ8.8������etaT^2/etaI^2����PBRTʵ�ʴ�����etaI^2/etaT^2���������ˡ�
	// ����ԭ��Ҳ���������顣
	// if (!Camera) f_transmittion *= (etaT * etaT) / (etaI * etaI);
	return f_transmittion;
}

Vector3 NXRMicrofacetReflection::f(const Vector3& wo, const Vector3& wi)
{
	Vector3 wh = wo + wi;
	wh.Normalize();
	// ʹ�þ���ֵȷ��wo��wi����n����ͬһ����
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
