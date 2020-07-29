#pragma once
#include "NXIntersection.h"

// дһ��PBR���ʡ�������Ⱦ��ʹ��PBR���ʡ�
// �����Ὣ��ͨ���ʺ�PBR���ʺϲ���
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

	NXPBRMaterial(const Vector3& Diffuse, const Vector3& Specular, const Vector3& Reflectivity, const Vector3& Refractivity, float Roughness, float IOR);
	~NXPBRMaterial() {}

	void CalcSampleProbabilities(float reflectance);

	Vector3 m_diffuse;
	Vector3 m_specular;
	Vector3 m_reflectivity;
	Vector3 m_refractivity;
	float m_roughness;
	float m_IOR;

	// Roulette����
	float m_probability;
	SampleProbabilities m_sampleProbs;
};
