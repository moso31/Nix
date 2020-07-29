#include "NXBSDF.h"
#include "NXIntersection.h"
#include "NXRandom.h"
#include "NXPBRMaterial.h"
#include "SamplerMath.h"

NXBSDF::NXBSDF(const NXHit& pHitInfo, const shared_ptr<NXPBRMaterial>& pMaterial) :
	ng(pHitInfo.normal),
	ns(pHitInfo.shading.normal),
	ss(pHitInfo.shading.dpdu),
	ts(ns.Cross(ss)),
	pMat(pMaterial)
{
	// ����ֱ�Ӱ�΢��ֲ���Fresnel���������ò�̫��
	// ΢��ֲ���Fresnelģ�ͷֱ𶼾��кܶ��֡��������Ըĳɸ����п���չ�Եķ���
	pDistrib = make_unique<NXRDistributionBeckmann>(pMat->m_roughness);
	pFresnelSpecular = make_unique<NXFresnelCommon>(pMat->m_specular);
	auto pFresnelReflectivity = make_unique<NXFresnelDielectric>(1.0f, pMat->m_IOR);

	Vector3 wo = WorldToReflection(pHitInfo.direction);
	m_reflectance = pFresnelReflectivity->FresnelReflectance(CosTheta(wo)).x;	// xyz��һ�� ȡ˭����
	pMat->CalcSampleProbabilities(m_reflectance);
}

Vector3 NXBSDF::Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf, shared_ptr<SampleEvents> o_sampleEvent)
{
	// �����������or�������䣬ִ��������+��������
	// ����Ǵ����䣬ִ�д�����
	// ����Ǵ����䣬ִ�д�����
	SampleEvents eEvent(NONE);
	float fRandom = NXRandom::GetInstance()->CreateFloat();
	if (fRandom < pMat->m_sampleProbs.Diffuse)
	{
		eEvent = DIFFUSE;
	}
	else if (fRandom < pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular)
	{
		eEvent = GLOSSY;
	}
	else if (fRandom < pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular + pMat->m_sampleProbs.Reflect)
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
	Vector3 wh = pDistrib->Sample_wh(wo);
	o_wi = Reflect(wo, wh);
	if (!IsSameHemisphere(wo, o_wi)) 
		return Vector3(0.0f);
	o_pdf = PdfSpecular(wo, wh);
	return pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, o_wi) * pFresnelSpecular->FresnelReflectance(o_wi.Dot(wh)) / (4.0f * AbsCosTheta(wo) * AbsCosTheta(o_wi));
}

Vector3 NXBSDF::EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	Vector3 wh = wo + wi;
	wh.Normalize();
	o_pdf = PdfSpecular(wo, wh);
	return pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, wi) * pFresnelSpecular->FresnelReflectance(wi.Dot(wh)) / (4.0f * AbsCosTheta(wo) * AbsCosTheta(wi));
}

Vector3 NXBSDF::SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
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
