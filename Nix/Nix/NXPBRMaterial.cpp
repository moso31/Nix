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
	m_rSS = albedo * (1.0f - metallic); 
	m_F0 = Vector3::Lerp(Vector3(0.04f), albedo, metallic); 

	m_diffuseAlbedo = m_rSS * (1.0f - m_refractivity);
	m_specularAlbedo = m_F0 * (1.0f - m_reflectivity);
	m_reflectAlbedo = m_albedo * m_reflectivity;
	m_refractAlbedo = m_albedo * m_refractivity;
}

/*
关于离线渲染各项BRDF的采样概率：按照
	kD = (1-F) * (1-T)
	kT = (1-F) *   T
	kS =   F   * (1-R)
	kR =   F   *   R
的比例进行采样。
*/
void NXPBRMaterial::CalcSampleProbabilities(float F)
{
	m_sampleProbs.Diffuse  = (1.0f - F) * (1.0f - m_refractivity);
	m_sampleProbs.Specular = F * (1.0f - m_reflectivity);
	m_sampleProbs.Reflect  = F * m_reflectivity;
	m_sampleProbs.Refract  = (1.0f - F) * m_refractivity;
}
