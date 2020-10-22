#pragma once
#include "NXIntersection.h"

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

	void CalcSampleProbabilities(float reflectance);

	Vector3 m_albedo;
	float m_metallic;
	float m_roughness;

	float m_reflectivity; // 镜面反射比 R
	float m_refractivity; // 镜面折射比 T
	float m_IOR; // 折射率（如果有折射项）

	// 采样概率
	SampleProbabilities m_sampleProbs;

	Vector3 GetF0() const { return m_F0; }
	//Vector3 GetDiffuseAlbedo() const { return m_diffuseAlbedo; }
	//Vector3 GetSpecularAlbedo() const { return m_specularAlbedo; }
	//Vector3 GetReflectAlbedo() const { return m_reflectAlbedo; }
	//Vector3 GetRefractAlbedo() const { return m_refractAlbedo; }

private:
	Vector3 m_F0;

	//Vector3 m_diffuseAlbedo;
	//Vector3 m_specularAlbedo;
	//Vector3 m_reflectAlbedo;
	//Vector3 m_refractAlbedo;
};
