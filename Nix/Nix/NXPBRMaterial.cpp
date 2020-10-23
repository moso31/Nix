#include "NXReflection.h"
#include "NXPBRMaterial.h"

NXPBRMaterial::NXPBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR) :
	m_albedo(albedo),
	m_metallic(metallic),
	m_roughness(roughness),
	m_reflectivity(reflectivity),
	m_refractivity(refractivity),
	m_IOR(IOR)
{
	// F0: ����Ƕ�Ϊ0��ʱ��Fresnel�����ʡ�
	m_F0 = Vector3::Lerp(Vector3(0.04f), albedo, metallic); 
}

void NXPBRMaterial::CalcSampleProbabilities(float F)
{
	m_sampleProbs.Diffuse  = (1.0f - F) * (1.0f - m_refractivity);
	m_sampleProbs.Specular = F * (1.0f - m_reflectivity);
	m_sampleProbs.Reflect  = F * m_reflectivity;
	m_sampleProbs.Refract  = (1.0f - F) * m_refractivity;
}

ConstantBufferMaterial NXPBRMaterial::GetConstantBuffer()
{
	ConstantBufferMaterial cb;
	cb.albedo = m_albedo;
	cb.metallic = m_metallic;
	cb.roughness = m_roughness;
	return cb;
}
