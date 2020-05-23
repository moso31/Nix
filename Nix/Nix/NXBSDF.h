#pragma once
#include "NXReflectionModel.h"

class NXHit;

class NXBSDF
{
public:
	NXBSDF(const NXHit& pHitInfo, float matIOR = 1.0f);
	~NXBSDF() {}

	void AddReflectionModel(const shared_ptr<NXReflectionModel>& pReflectionModel);

	Vector3 f(const Vector3& woWorld, const Vector3& wiWorld, ReflectionType reflectType = REFLECTIONTYPE_ALL);
	Vector3 Sample_f(const Vector3& woWorld, Vector3& outwiWorld, float& pdf, ReflectionType reflectType = REFLECTIONTYPE_ALL);

	Vector3 WorldToReflection(const Vector3& p);
	Vector3 ReflectionToWorld(const Vector3& p);

private:
	vector<shared_ptr<NXReflectionModel>> m_reflectionModels;
	Vector3 ng;
	Vector3 ns, ss, ts;
	Vector3 eta;	// ’€…‰≤ƒ÷ µƒIOR
};
