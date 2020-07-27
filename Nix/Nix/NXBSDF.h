#pragma once
#include "NXReflectionModel.h"

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
		SPECULAR = 2,
		REFLECT = 4,
		REFRACT = 8,
	};

	NXBSDF(const NXHit& pHitInfo, const shared_ptr<NXPBRMaterial>& pMaterial);
	~NXBSDF() {}

	Vector3 Sample(const Vector3& woWorld, Vector3& o_wiWorld, float& o_pdf);
	Vector3 Evaluate(const Vector3& woWorld, const Vector3& wiWorld, float& o_pdf);
	Vector3 SampleDiffuse(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 EvaluateDiffuse(const Vector3& wo, const Vector3& wi, float& o_pdf);
	Vector3 SampleSpecular(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 EvaluateSpecular(const Vector3& wo, const Vector3& wi, float& o_pdf);
	Vector3 SampleReflect(const Vector3& wo, Vector3& o_wi, float& o_pdf);
	Vector3 SampleRefract(const Vector3& wo, Vector3& o_wi, float& o_pdf);

	float PdfDiffuse(const Vector3& wo, const Vector3& wi);
	float PdfSpecular(const Vector3& wo, const Vector3& wh);	// 注意计算Specular使用半向量 wh！

	Vector3 WorldToReflection(const Vector3& p);
	Vector3 ReflectionToWorld(const Vector3& p);

private:
	Vector3 ng;
	Vector3 ns, ss, ts;
	shared_ptr<NXPBRMaterial> pMat;
	unique_ptr<NXRDistributionBeckmann> pDistrib;
	unique_ptr<NXFresnelCommon> pFresnel;
};
