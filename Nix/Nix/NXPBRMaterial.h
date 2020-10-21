#pragma once
#include "NXIntersection.h"

class NXPBRMaterial
{
public:
	/*
	Nix使用Metal/Albedo/Roughness工作流。数学上遵从以下方案：
	首先用albeto和metallic确定diffuse和specular的值。
	同时依照菲涅尔反射率可以确定特定角度的入射光反射比——即F(degree)。
	然后在材质中放置一个镜面反射比R，代表反射能量F中被镜面反射的部分，剩余部分参与Specular光滑反射。
	最后在材质中放置一个镜面折射比T，代表折射能量(1-F)中被镜面折射的部分，剩余部分参与diffuse漫反射。
	最终的BRDF计算结果为：
		f = kD·fD + kS·fS + kR·fR + kT·fT
	其中：
		kD = (1-F) * (1-T)
		kT = (1-F) *   T
		kS =   F   * (1-R)
		kR =   F   *   R

	以上方案仅用于离线渲染。
	实时渲染时，通常不在BRDF计算时考虑镜面内容，所以R=T=0。
	*/

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
	Vector3 m_metallic;
	float m_roughness;

	float m_reflectivity; // 镜面反射比 R
	float m_refractivity; // 镜面折射比 T
	float m_IOR; // 折射率（如果有折射项）

	// 采样概率
	SampleProbabilities m_sampleProbs;

	Vector3 GetSubsurfaceAlbedo() const { return m_rSS; }
	Vector3 GetF0() const { return m_F0; }
	Vector3 GetDiffuseAlbedo() const { return m_diffuseAlbedo; }
	Vector3 GetSpecularAlbedo() const { return m_specularAlbedo; }
	Vector3 GetReflectAlbedo() const { return m_reflectAlbedo; }
	Vector3 GetRefractAlbedo() const { return m_refractAlbedo; }

private:
	Vector3 m_rSS;
	Vector3 m_F0;

	Vector3 m_diffuseAlbedo;
	Vector3 m_specularAlbedo;
	Vector3 m_reflectAlbedo;
	Vector3 m_refractAlbedo;
};
