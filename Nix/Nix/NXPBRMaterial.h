#pragma once
#include "NXIntersection.h"
#include "ShaderStructures.h"

class NXPBRMaterial
{
public:
	// ��������
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

	float m_reflectivity; // ���淴��� R
	float m_refractivity; // ��������� T
	float m_IOR; // �����ʣ�����������

	// ��������
	SampleProbabilities m_sampleProbs;

private:
	Vector3 m_F0;
};
