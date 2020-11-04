#include "NXBSDF.h"
#include "NXIntersection.h"
#include "NXRandom.h"
#include "NXPBRMaterial.h"
#include "SamplerMath.h"

NXBSDF::NXBSDF(const NXHit& pHitInfo, NXPBRMaterial* pMaterial) :
	ng(pHitInfo.normal),
	ns(pHitInfo.shading.normal),
	ss(pHitInfo.shading.dpdu),
	ts(ns.Cross(ss)),
	pMat(pMaterial)
{
	pDistrib = std::make_unique<NXRDistributionBeckmann>(pMat->m_roughness);
	pFresnel = std::make_unique<NXFresnelCommon>(pMat->GetF0());

	// 计算BSDF的采样概率（结合入射角度wo）
	Vector3 wo = WorldToReflection(pHitInfo.direction);
	float F = pFresnel->FresnelReflectance(CosTheta(wo)).GetGrayValue();
	pMat->CalcSampleProbabilities(F); 
}

Vector3 NXBSDF::Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf, SampleEvents* o_sampleEvent)
{
	// 按各种基本分布的概率选取采样事件。
	// 目前只有四种基本分布：漫反射/光滑反射/纯反射/纯折射
	SampleEvents eEvent(NONE);
	float fRandom = NXRandom::GetInstance().CreateFloat();
	if (fRandom <= pMat->m_sampleProbs.Diffuse)
		eEvent = DIFFUSE;
	else if (fRandom <= pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular)
		eEvent = GLOSSY;
	else if (fRandom <= pMat->m_sampleProbs.Diffuse + pMat->m_sampleProbs.Specular + pMat->m_sampleProbs.Reflect)
		eEvent = REFLECT;
	else 
		eEvent = REFRACT;

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
	// 不考虑不可能被采样到的Delta BRDF分布（即纯反射/纯折射）。
	// 所以f = kDfD + kSfS

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
	Vector2 u = NXRandom::GetInstance().CreateVector2();
	o_wi = SamplerMath::CosineSampleHemisphere(u);
	if (wo.z < 0.0f) 
		o_wi.z *= -1.0f;
	o_pdf = PdfDiffuse(wo, o_wi);
	return pMat->m_albedo * (1.0f - pMat->m_metallic) / XM_PI;
}

Vector3 NXBSDF::EvaluateDiffuse(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	o_pdf = PdfDiffuse(wo, wi);
	return pMat->m_albedo * (1.0f - pMat->m_metallic) / XM_PI;
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
	return pDistrib->D(wh) * pDistrib->G(wo, o_wi) * pFresnel->FresnelReflectance(o_wi.Dot(wh)) / (4.0f * absCosThetaO * absCosThetaI);
}

Vector3 NXBSDF::EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf)
{
	float absCosThetaO = AbsCosTheta(wo);
	float absCosThetaI = AbsCosTheta(wi);
	if (absCosThetaO == 0.0f || absCosThetaI == 0.0f) return Vector3(0.0f);

	Vector3 wh = wo + wi;
	wh.Normalize();
	o_pdf = PdfSpecular(wo, wh);
	Vector3 result = pDistrib->D(wh) * pDistrib->G(wo, wi) * pFresnel->FresnelReflectance(wi.Dot(wh)) / (4.0f * absCosThetaO * absCosThetaI);

	return result;
}

Vector3 NXBSDF::SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	if (wo.z == 0.0f) return Vector3(0.0f);

	o_wi = Vector3(-wo.x, -wo.y, wo.z);
	o_pdf = pMat->m_sampleProbs.Reflect;

	Vector3 F = pFresnel->FresnelReflectance(CosTheta(wo));
	return pMat->m_albedo * F / AbsCosTheta(o_wi);
}

Vector3 NXBSDF::SampleRefract(const Vector3& wo, Vector3& o_wi, float& o_pdf)
{
	bool bEntering = CosTheta(wo) > 0;
	float etaI = bEntering ? 1.0f : pMat->m_IOR;
	float etaT = bEntering ? pMat->m_IOR : 1.0f;

	Vector3 normal = Vector3(0.0f, 0.0f, bEntering ? 1.0f : -1.0f);
	if (!Refract(wo, normal, etaI, etaT, o_wi))
		return Vector3(0.0f);	// 全内反射

	o_pdf = pMat->m_sampleProbs.Refract;
	Vector3 F = pFresnel->FresnelReflectance(CosTheta(wo));
	Vector3 transmitivity = pMat->m_albedo * (1.0f - F.GetGrayValue()) / AbsCosTheta(o_wi);

	bool IsFromCamera = true;
	// 如果是从相机出发的射线(PT)，考虑折射过程中微分角压缩比。
	// 如果是从光源出发(PM)，则此项正好和伴随BSDF相互抵消，不用考虑。
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
	// Diffuse使用的是余弦采样。
	if (IsSameHemisphere(wo, wi)) 
		return AbsCosTheta(wi) * XM_1DIVPI * pMat->m_sampleProbs.Diffuse;
	return 0.0f;
}

float NXBSDF::PdfSpecular(const Vector3& wo, const Vector3& wh)
{
	// Specular使用D分布进行重点采样。根据文章
	// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
	// 的说法，除以一个4*wo*wh可以获得更好的采样结果。
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

void NXBSDF::Release()
{
}
