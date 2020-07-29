#pragma once
#include "NXIntersection.h"

// 写一个PBR材质。离线渲染先使用PBR材质。
// 将来会将普通材质和PBR材质合并。
class NXPBRMaterial
{
public:
	// 采样概率
	struct SampleProbabilities
	{
		float Diffuse;
		float Specular;
		float Reflect;
		float Refract;
	};

	NXPBRMaterial(const Vector3& Diffuse, const Vector3& Specular, const Vector3& Reflectivity, const Vector3& Refractivity, float Roughness, float IOR);
	~NXPBRMaterial() {}

	void CalcSampleProbabilities(float reflectance);

	Vector3 m_diffuse;
	Vector3 m_specular;
	Vector3 m_reflectivity;
	Vector3 m_refractivity;
	float m_roughness;
	float m_IOR;

	// Roulette概率
	float m_probability;
	SampleProbabilities m_sampleProbs;
};
