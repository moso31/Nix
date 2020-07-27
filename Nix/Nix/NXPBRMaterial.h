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
		float Diff;
		float Spec;
	};

	NXPBRMaterial(const Vector3& Diffuse, const Vector3& Specular, const Vector3& Reflectivity, float Roughness, float IOR);
	~NXPBRMaterial() {}

	void CalcSampleProbabilities();

	Vector3 m_diffuse;
	Vector3 m_specular;
	Vector3 m_reflectivity;
	float m_roughness;
	float m_IOR;

	// Roulette概率
	float m_probability;
	SampleProbabilities m_sampleProbs;
};
