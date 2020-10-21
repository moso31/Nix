#pragma once
#include "NXIntersection.h"

class NXPBRMaterial
{
public:
	/*
	Nixʹ��Metal/Albedo/Roughness����������ѧ��������·�����
	������albeto��metallicȷ��diffuse��specular��ֵ��
	ͬʱ���շ����������ʿ���ȷ���ض��Ƕȵ�����ⷴ��ȡ�����F(degree)��
	Ȼ���ڲ����з���һ�����淴���R������������F�б����淴��Ĳ��֣�ʣ�ಿ�ֲ���Specular�⻬���䡣
	����ڲ����з���һ�����������T��������������(1-F)�б���������Ĳ��֣�ʣ�ಿ�ֲ���diffuse�����䡣
	���յ�BRDF������Ϊ��
		f = kD��fD + kS��fS + kR��fR + kT��fT
	���У�
		kD = (1-F) * (1-T)
		kT = (1-F) *   T
		kS =   F   * (1-R)
		kR =   F   *   R

	���Ϸ���������������Ⱦ��
	ʵʱ��Ⱦʱ��ͨ������BRDF����ʱ���Ǿ������ݣ�����R=T=0��
	*/

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
	Vector3 m_metallic;
	float m_roughness;

	float m_reflectivity; // ���淴��� R
	float m_refractivity; // ��������� T
	float m_IOR; // �����ʣ�����������

	// ��������
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
