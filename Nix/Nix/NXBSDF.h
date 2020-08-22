#pragma once
#include "NXDistribution.h"
#include "NXFresnel.h"
#include "NXReflection.h"

using namespace NXReflection;

class NXHit;
class NXPBRMaterial;

class NXBSDF
{
public:
	enum SampleEvents
	{
		NONE = 0,
		DIFFUSE = 1,
		GLOSSY = 2,
		REFLECT = 4,
		REFRACT = 8,
		DELTA = (REFLECT | REFRACT),
		NONDELTA = (DIFFUSE | GLOSSY),
		ALL = (DELTA | NONDELTA)
	};

	NXBSDF(const NXHit& pHitInfo, const std::shared_ptr<NXPBRMaterial>& pMaterial);
	~NXBSDF() {}

	Vector3 Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf, std::shared_ptr<SampleEvents> o_sampleEvent = nullptr);
	Vector3 Evaluate(const Vector3& woWorld, const Vector3& wiWorld, float& o_pdf);
	Vector3 SampleDiffuse(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 EvaluateDiffuse(const Vector3& wo, const Vector3& wi, float& o_pdf);
	Vector3 SampleSpecular(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf);
	Vector3 SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 SampleRefract(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 SampleDiffuseWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf);
	Vector3 SampleReflectWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf);
	Vector3 SampleRefractWorld(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf);

	float PdfDiffuse(const Vector3& wo, const Vector3& wi);
	float PdfSpecular(const Vector3& wo, const Vector3& wh);	// 注意计算Specular使用半向量 wh！

	Vector3 WorldToReflection(const Vector3& p);
	Vector3 ReflectionToWorld(const Vector3& p);

private:
	Vector3 ng;
	Vector3 ns, ss, ts;
	std::shared_ptr<NXPBRMaterial> pMat;
	std::unique_ptr<NXRDistributionBeckmann> pDistrib;
	std::unique_ptr<NXFresnel> pFresnelSpecular;

	float m_reflectance; // 对镜面反射，可以不记录Fresnel模型，直接记录反射率。
};
