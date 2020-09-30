#include "NXBSDF.h"
#include "NXIntersection.h"
#include "NXRandom.h"
#include "NXPBRMaterial.h"
#include "SamplerMath.h"

NXBSDF::NXBSDF(const NXHit& pHitInfo, const std::shared_ptr<NXPBRMaterial>& pMaterial) :
	ng(pHitInfo.normal),
	ns(pHitInfo.shading.normal),
	ss(pHitInfo.shading.dpdu),
	ts(ns.Cross(ss)),
	pMat(pMaterial)
{
	// ����ֱ�Ӱ�΢��ֲ���Fresnel���������ò�̫��
	// ΢��ֲ���Fresnelģ�ͷֱ𶼾��кܶ��֡��������Ըĳɸ����п���չ�Եķ���
	pDistrib = std::make_unique<NXRDistributionBeckmann>(pMat->m_roughness);
	pFresnelSpecular = std::make_unique<NXFresnelCommon>(pMat->m_specular);
	auto pFresnelReflectivity = std::make_unique<NXFresnelDielectric>(1.0f, pMat->m_IOR);

	Vector3 wo = WorldToReflection(pHitInfo.direction);
	m_reflectance = pMat->m_IOR > 0.0f ? pFresnelReflectivity->FresnelReflectance(CosTheta(wo)).x : 1.0f;	// �з����ʱ�ȻΪ����ʣ�xyz��һ�� ȡ˭����
	pMat->CalcSampleProbabilities(m_reflectance);
}

Vector3 NXBSDF::Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf, std::shared_ptr<SampleEvents> o_sampleEvent)
{
	// �����������or�������䣬ִ��������+��������
	// ����Ǵ����䣬ִ�д�����
	// ����Ǵ����䣬ִ�д�����
	SampleEvents eEvent(NONE);
	float fRandom = NXRandom::GetInstance()->CreateFloat();
	if (fRandom <= pMat->m_sampleProbs.Diffuse)
	{
		eEvent = DIFFUSE;
	}
	else if (fRandom <= pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular)
	{
		eEvent = GLOSSY;
	}
	else if (fRandom <= pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular + pMat->m_sampleProbs.Reflect)
	{
		eEvent = REFLECT;
	}
	else 
	{
		eEvent = REFRACT;
	}

	if (o_sampleEvent)
		*o_sampleEvent = eEvent;

	Vector3 result(0.0f);
	Vector3 wo = WorldToReflection(woWorld);
	Vector3 wi;
	o_pdf = 0.0f;
	float pdfD, pdfS;
	switch (eEvent)
	{
	case NXBSDF::DIFFUSE:
		result += SampleDiffuse(wo, wi, pdfD);
		if (result.IsZero())
			return Vector3(0.0f);
		result += EvaluateSpecular(wo, wi, pdfS);
		o_pdf = pdfD + pdfS;
		break;
	case NXBSDF::GLOSSY:
		result += SampleSpecular(wo, wi, pdfD);
		if (result.IsZero())
			return Vector3(0.0f);
		result += EvaluateDiffuse(wo, wi, pdfS);
		o_pdf = pdfD + pdfS;
		break;
	case NXBSDF::REFLECT:
		result += SampleReflect(wo, wi, o_pdf);
		break;
	case NXBSDF::REFRACT:
		result += SampleRefract(wo, wi, o_pdf);
		break;
	}

	//if (o_pdf == 0.0f)
	//{
	//	printf("wo: %f %f %f\n", wo.x, wo.y, wo.z);
	//	printf("wi: %f %f %f\n", wi.x, wi.y, wi.z);
	//	printf("result: %f %f %f\n", result.x, result.y, result.z);
	//	printf("eEvent: %d\n", eEvent);
	//	printf("sampleProb: Diffuse: %f Specular: %f Reflect: %f Refract:%f\n", pMat->m_sampleProbs.Diffuse, pMat->m_sampleProbs.Specular, pMat->m_sampleProbs.Reflect, pMat->m_sampleProbs.Refract);
	//	printf("fRandom: %f\n", fRandom);
	//}

	o_wiWorld = ReflectionToWorld(wi);
	return result;
}

Vector3 NXBSDF::Evaluate(const Vector3& woWorld, const Vector3& wiWorld, float& o_pdf)
{
	Vector3 result(0.0f);
	Vector3 wo = WorldToReflection(woWorld);
	Vector3 wi = WorldToReflection(wiWorld);
	float pdfD, pdfS;
	result += EvaluateDiffuse(wo, wi, pdfD);
	result += EvaluateSpecular(wo, wi, pdfS);
	if (Vector3::IsNaN(result))
	{
		Vector3 D = EvaluateDiffuse(wo, wi, pdfD);
		Vector3 S = EvaluateSpecular(wo, wi, pdfS);
		printf("pMat->m_diffuse: %f %f %f\n", pMat->m_diffuse.x, pMat->m_diffuse.y, pMat->m_diffuse.z);
		printf("pMat->m_specular: %f %f %f\n", pMat->m_specular.x, pMat->m_specular.y, pMat->m_specular.z);
	}
	o_pdf = pdfD + pdfS;
	return result;
}

Vector3 NXBSDF::SampleDiffuse(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	o_wi = SamplerMath::CosineSampleHemisphere(u);
	if (wo.z < 0.0f) 
		o_wi.z *= -1.0f;
	o_pdf = PdfDiffuse(wo, o_wi);
	return pMat->m_diffuse / XM_PI;
}

Vector3 NXBSDF::EvaluateDiffuse(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	o_pdf = PdfDiffuse(wo, wi);
	return pMat->m_diffuse / XM_PI;
}

Vector3 NXBSDF::SampleSpecular(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	float absCosThetaO = AbsCosTheta(wo);
	if (absCosThetaO == 0.0f) return Vector3(0.0f);

	Vector3 wh = pDistrib->Sample_wh(wo);
	o_wi = Reflect(wo, wh);
	float absCosThetaI = AbsCosTheta(o_wi);

	if (absCosThetaI == 0.0f || !IsSameHemisphere(wo, o_wi)) return Vector3(0.0f);

	o_pdf = PdfSpecular(wo, wh);
	return pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, o_wi) * pFresnelSpecular->FresnelReflectance(o_wi.Dot(wh)) / (4.0f * absCosThetaO * absCosThetaI);
}

Vector3 NXBSDF::EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	float absCosThetaO = AbsCosTheta(wo);
	float absCosThetaI = AbsCosTheta(wi);
	if (absCosThetaO == 0.0f || absCosThetaI == 0.0f) return Vector3(0.0f);

	Vector3 wh = wo + wi;
	wh.Normalize();
	o_pdf = PdfSpecular(wo, wh);
	Vector3 result = pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, wi) * pFresnelSpecular->FresnelReflectance(wi.Dot(wh)) / (4.0f * absCosThetaO * absCosThetaI);

	if (Vector3::IsNaN(result))
	{
		auto F = pFresnelSpecular->FresnelReflectance(wi.Dot(wh));
		printf("pMat->m_specular: %f %f %f\n", pMat->m_specular.x, pMat->m_specular.y, pMat->m_specular.z);
		printf("pDistrib->D: %f G: %f F: %f %f %f\n", pDistrib->D(wh), pDistrib->G(wo, wi), F.x, F.y, F.z);
		printf("wo: %f %f %f\n", wo.x, wo.y, wo.z);
		printf("wi: %f %f %f\n", wi.x, wi.y, wi.z);
		printf("wh: %f %f %f\n", wh.x, wh.y, wh.z);
	}

	return result;
}

Vector3 NXBSDF::SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	if (wo.z == 0.0f) return Vector3(0.0f);

	o_wi = Vector3(-wo.x, -wo.y, wo.z);
	o_pdf = pMat->m_sampleProbs.Reflect;
	return pMat->m_reflectivity * m_reflectance / AbsCosTheta(o_wi);
}

Vector3 NXBSDF::SampleRefract(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	bool bEntering = CosTheta(wo) > 0;
	float etaI = bEntering ? 1.0f : pMat->m_IOR;
	float etaT = bEntering ? pMat->m_IOR : 1.0f;

	Vector3 normal = Vector3(0.0f, 0.0f, bEntering ? 1.0f : -1.0f);
	if (!Refract(wo, normal, etaI, etaT, o_wi))
		return Vector3(0.0f);	// ȫ�ڷ���

	o_pdf = pMat->m_sampleProbs.Refract;
	Vector3 transmitivity = pMat->m_refractivity * (1.0f - m_reflectance) / AbsCosTheta(o_wi);

	bool IsFromCamera = true;
	// ����Ǵ��������������(PT)���������������΢�ֽ�ѹ���ȡ�
	// ����Ǵӹ�Դ����(PM)����������úͰ���BSDF�໥���������ÿ��ǡ�
	if (IsFromCamera) transmitivity *= etaI * etaI / (etaT * etaT);
	return transmitivity;
}

Vector3 NXBSDF::SampleDiffuseWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf)
{
	Vector3 wo = WorldToReflection(woWorld), wi;
	Vector3 result = SampleDiffuse(wo, wi, o_pdf);
	o_wiWorld = ReflectionToWorld(wi);
	return result;
}

Vector3 NXBSDF::SampleReflectWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf)
{
	if (pMat->m_sampleProbs.Reflect == 0.0f)
	{
		o_pdf = 0.0f;
		return Vector3(0.0f);
	}
	Vector3 wo = WorldToReflection(woWorld), wi;
	Vector3 result = SampleReflect(wo, wi, o_pdf);
	o_wiWorld = ReflectionToWorld(wi);
	return result;
}

Vector3 NXBSDF::SampleRefractWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf)
{
	if (pMat->m_sampleProbs.Refract == 0.0f)
	{
		o_pdf = 0.0f;
		return Vector3(0.0f);
	}
	Vector3 wo = WorldToReflection(woWorld), wi;
	Vector3 result = SampleRefract(wo, wi, o_pdf);
	o_wiWorld = ReflectionToWorld(wi);
	return result;
}

float NXBSDF::PdfDiffuse(const Vector3& wo, const Vector3& wi)
{
	// Diffuseʹ�õ������Ҳ�����
	if (IsSameHemisphere(wo, wi)) 
		return AbsCosTheta(wi) * XM_1DIVPI * pMat->m_sampleProbs.Diffuse;
	return 0.0f;
}

float NXBSDF::PdfSpecular(const Vector3& wo, const Vector3& wh)
{
	// Specularʹ��D�ֲ������ص��������������
	// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
	// ��˵��������һ��4*wo*wh���Ի�ø��õĲ��������
	return pDistrib->Pdf(wh) / (4.0f * wo.Dot(wh)) * pMat->m_sampleProbs.Specular;
}

Vector3 NXBSDF::WorldToReflection(const Vector3& p)
{
	return Vector3(p.Dot(ss), p.Dot(ts), p.Dot(ns));
}

Vector3 NXBSDF::ReflectionToWorld(const Vector3& p)
{
	return Vector3(
		p.x * ss.x + p.y * ts.x + p.z * ns.x,
		p.x * ss.y + p.y * ts.y + p.z * ns.y,
		p.x * ss.z + p.y * ts.z + p.z * ns.z);
}
