#pragma once
#include "NXFresnel.h"
#include "NXDistribution.h"

namespace NXReflection
{
	Vector3 Reflect(const Vector3& dirIn, const Vector3& dirNormal);
	bool Refract(const Vector3& dirIn, const Vector3& dirNormal, float etaI, float etaT, Vector3& outRefract);

	float CosTheta(const Vector3& w);
	float AbsCosTheta(const Vector3& w);
	float Cos2Theta(const Vector3& w);
	float Sin2Theta(const Vector3& w);
	float SinTheta(const Vector3& w);
	float Tan2Theta(const Vector3& w);
	float TanTheta(const Vector3& w);

	bool IsSameHemisphere(const Vector3& wo, const Vector3& wi);
}

enum ReflectionType
{
	REFLECTIONTYPE_DIFFUSE = 1 << 0,
	REFLECTIONTYPE_SPECULAR = 1 << 1,
	REFLECTIONTYPE_GLOSSY = 1 << 2,
	REFLECTIONTYPE_REFLECTION = 1 << 3,
	REFLECTIONTYPE_TRANSMISSION = 1 << 4,
	REFLECTIONTYPE_ALL = REFLECTIONTYPE_DIFFUSE | REFLECTIONTYPE_SPECULAR | REFLECTIONTYPE_GLOSSY | REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_TRANSMISSION
};

class NXReflectionModel
{
public:
	NXReflectionModel(ReflectionType type) : m_type(type) {}
	~NXReflectionModel() {}

	virtual Vector3 f(const Vector3& wo, const Vector3& wi) = 0;
	virtual Vector3 Sample_f(const Vector3& wo, Vector3& wi, float& pdf);
	virtual float Pdf(const Vector3& wo, const Vector3& wi);

	ReflectionType GetReflectionType() { return m_type; }
	// 是否匹配指定类型。type各bit全部相同时视作匹配。
	bool IsMatchingType(ReflectionType type);

protected:
	ReflectionType m_type;
};

class NXRLambertianReflection : public NXReflectionModel
{
public:
	NXRLambertianReflection(const Vector3& R) : 
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_DIFFUSE)),
		R(R) {}
	~NXRLambertianReflection() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) override;

private:
	Vector3 R;
};

class NXRPrefectReflection : public NXReflectionModel
{
public:
	NXRPrefectReflection(const Vector3& R, const shared_ptr<NXFresnel>& fresnel) :
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_SPECULAR)), 
		R(R), fresnel(fresnel) {}
	~NXRPrefectReflection() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) override { return Vector3(0.0f); }
	Vector3 Sample_f(const Vector3& wo, Vector3& wi, float& pdf) override;

	// 作为delta分布数学模型，完美反射的pdf实现比较特殊，
	// 未被选为样本时其pdf=0，被选中为样本（计算Sample_f）时其pdf=1。
	float Pdf(const Vector3& wo, const Vector3& wi) override { return 0.0f; }

private:
	Vector3 R;
	shared_ptr<NXFresnel> fresnel;
};

class NXRPrefectTransmission : public NXReflectionModel
{
public:
	NXRPrefectTransmission(const Vector3& T, float etaA, float etaB, bool IsFromCamera) :
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_TRANSMISSION | REFLECTIONTYPE_SPECULAR)),
		T(T), etaA(etaA), etaB(etaB), fresnel(etaA, etaB), IsFromCamera(IsFromCamera) {}
	~NXRPrefectTransmission() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) override { return Vector3(0.0f); }
	Vector3 Sample_f(const Vector3& wo, Vector3& wi, float& pdf) override;

	// 作为delta分布数学模型，完美折射的pdf实现比较特殊，
	// 未被选为样本时其pdf=0，被选中为样本（计算Sample_f）时其pdf=1。
	float Pdf(const Vector3& wo, const Vector3& wi) override { return 0.0f; }

private:
	Vector3 T;
	NXFresnelDielectric fresnel;	// 导体不可能折射
	float etaA, etaB;
	bool IsFromCamera;
};

class NXRMicrofacetReflection : public NXReflectionModel
{
public:
	NXRMicrofacetReflection(const Vector3& R, const shared_ptr<NXRDistribution>& distrib, const shared_ptr<NXFresnel> fresnel) :
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_GLOSSY | REFLECTIONTYPE_REFLECTION)),
		R(R), distrib(distrib), fresnel(fresnel) {}
	~NXRMicrofacetReflection() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) override;
	Vector3 Sample_f(const Vector3& wo, Vector3& wi, float& pdf) override;
	float Pdf(const Vector3& wo, const Vector3& wi) override;

private:
	Vector3 R;
	shared_ptr<NXRDistribution> distrib;
	shared_ptr<NXFresnel> fresnel;
};

class NXRMicrofacetTransmission : public NXReflectionModel
{
public:
};
