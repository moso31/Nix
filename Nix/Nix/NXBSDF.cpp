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
	// 这里直接把微面分布和Fresnel放在外面用不太好
	// 微面分布和Fresnel模型分别都具有很多种。将来可以改成更具有可扩展性的方案
	pDistrib = make_unique<NXRDistributionBeckmann>(pMat->m_roughness);
	pFresnel = make_unique<NXFresnelCommon>(pMat->m_specular);
}

Vector3 NXBSDF::Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf)
{
	// 如果是漫反射or高亮反射，执行漫反射+高亮反射
	// 如果是纯反射，执行纯反射
	// 如果是纯折射，执行纯折射

	SampleEvents eEvent(NONE);
	float fRandom = NXRandom::GetInstance()->CreateFloat();
	if (fRandom < pMat->m_sampleProbs.Diff)
	{
		eEvent = DIFFUSE;
	}
	else if (fRandom < pMat->m_sampleProbs.Spec) 
	{
		eEvent = SPECULAR;
	}
	else
	{
		// absorb.
	}

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
	case NXBSDF::SPECULAR:
		result += SampleSpecular(wo, wi, pdfD);
		if (result.IsZero())
			return Vector3(0.0f);
		result += EvaluateDiffuse(wo, wi, pdfS);
		o_pdf = pdfD + pdfS;
		break;
	case NXBSDF::REFLECT:
		// 暂未实现
		break;
	case NXBSDF::REFRACT:
		// 暂未实现
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
	return pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, o_wi) * pFresnel->FresnelReflectance(o_wi.Dot(wh)) / (4.0f * AbsCosTheta(wo) * AbsCosTheta(o_wi));
}

Vector3 NXBSDF::EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	Vector3 wh = wo + wi;
	wh.Normalize();
	o_pdf = PdfSpecular(wo, wh);
	return pMat->m_specular * pDistrib->D(wh) * pDistrib->G(wo, wi) * pFresnel->FresnelReflectance(wi.Dot(wh)) / (4.0f * AbsCosTheta(wo) * AbsCosTheta(wi));
}

Vector3 NXBSDF::SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	o_wi = Vector3(-wo.x, -wo.y, wo.z);
	o_pdf = 1.0f;	// 完美反射模型被选中时pdf=1，未被选中时pdf=0
	return Vector3(pFresnel->FresnelReflectance(CosTheta(o_wi)) * pMat->m_ / abs(CosTheta(o_wi)));
}

Vector3 NXBSDF::SampleRefract(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	return Vector3();
}

float NXBSDF::PdfDiffuse(const Vector3& wo, const Vector3& wi)
{
	// Diffuse使用的是余弦采样。
	if (IsSameHemisphere(wo, wi)) 
		return AbsCosTheta(wi) * XM_1DIVPI * pMat->m_sampleProbs.Diff;
	return 0.0f;
}

float NXBSDF::PdfSpecular(const Vector3& wo, const Vector3& wh)
{
	// Specular使用D分布进行重点采样。根据文章
	// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
	// 的说法，除以一个4*wo*wh可以获得更好的采样结果。
	return pDistrib->Pdf(wh) / (4.0f * wo.Dot(wh)) * pMat->m_sampleProbs.Spec;
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
