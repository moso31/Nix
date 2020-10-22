#pragma once
#include "NXIntersection.h"

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

	void CalcSampleProbabilities(float reflectance);

	Vector3 m_albedo;
	float m_metallic;
	float m_roughness;

	float m_reflectivity; // ���淴��� R
	float m_refractivity; // ��������� T
	float m_IOR; // �����ʣ�����������

	// ��������
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
