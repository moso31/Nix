#pragma once
#include "NXIntersection.h"
#include "ShaderStructures.h"

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

	NXPBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR);
	~NXPBRMaterial() {}

	Vector3 GetF0() const { return m_F0; }
	void CalcSampleProbabilities(float reflectance);

	// DirectX
	ConstantBufferMaterial GetConstantBuffer();

public:

	Vector3 m_albedo;
	float m_metallic;
	float m_roughness;

	float m_reflectivity; // 镜面反射比 R
	float m_refractivity; // 镜面折射比 T
	float m_IOR; // 折射率（如果有折射项）

	// 采样概率
	SampleProbabilities m_sampleProbs;

private:
	Vector3 m_F0;
};
