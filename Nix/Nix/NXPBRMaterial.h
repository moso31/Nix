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

	// Roulette����
	float m_probability;
	SampleProbabilities m_sampleProbs;
};
