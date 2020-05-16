#pragma once
#include "Header.h"

bool Refract(const Vector3& dirIn, const Vector3& dirNormal, float etaI, float etaT, Vector3& outRefract);

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
	virtual Vector3 Sample_f(const Vector3& wo, Vector3& wi) = 0;

protected:
	ReflectionType m_type;
};

class NXRLambertianReflect : public NXReflectionModel
{
public:
	NXRLambertianReflect(const Vector3& R) : 
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_DIFFUSE)),
		R(R) {}
	~NXRLambertianReflect() {}

	Vector3 f(const Vector3& wo, const Vector3& wi);
	Vector3 Sample_f(const Vector3& wo, Vector3& wi);

private:
	Vector3 R;
};

class NXRPrefectReflect : public NXReflectionModel
{
public:
	NXRPrefectReflect(const Vector3& R) :
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_SPECULAR)), 
		R(R) {}
	~NXRPrefectReflect() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) { return Vector3(0.0f); }
	Vector3 Sample_f(const Vector3& wo, Vector3& wi);

private:
	Vector3 R;
};

class NXRPrefectTransmission : public NXReflectionModel
{
public:
	NXRPrefectTransmission(const Vector3& T, float etaI, float etaT) :
		NXReflectionModel(ReflectionType(REFLECTIONTYPE_TRANSMISSION | REFLECTIONTYPE_SPECULAR)),
		T(T), etaA(etaA), etaB(etaB) {} {}
	~NXRPrefectTransmission() {}

	Vector3 f(const Vector3& wo, const Vector3& wi) { return Vector3(0.0f); }
	Vector3 Sample_f(const Vector3& wo, Vector3& wi);

private:
	Vector3 T;
	float etaA, etaB; 
};

class NXRMicrofacetReflect : public NXReflectionModel
{
public:
};

class NXRMicrofacetTransmission : public NXReflectionModel
{
public:
};
