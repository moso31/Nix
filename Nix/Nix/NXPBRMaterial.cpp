#include "NXReflectionModel.h"
#include "NXPBRMaterial.h"

NXPBRMaterial::NXPBRMaterial(const Vector3& Diffuse, const Vector3& Specular, const Vector3& Reflectivity, const Vector3& Refractivity, float Roughness, float IOR) :
	m_diffuse(Diffuse),
	m_specular(Specular),
	m_reflectivity(Reflectivity),
	m_refractivity(Refractivity),
	m_roughness(Roughness),
	m_IOR(IOR)
{
}

void NXPBRMaterial::CalcSampleProbabilities(float reflectance)
{
	Vector3 reflectivity = reflectance * m_reflectivity;
	Vector3 refractivity = m_IOR > 0.0f ? (1.0f - reflectance) * m_refractivity : Vector3(0.0f);
	float prob = (m_diffuse + m_specular + reflectivity + refractivity).MaxComponent();
	m_probability = Clamp(prob, 0.0f, 1.0f);

	float sumDiffuse = m_diffuse.x + m_diffuse.y + m_diffuse.z;
	float sumSpecular = m_specular.x + m_specular.y + m_specular.z;
	float sumReflect = reflectivity.x + reflectivity.y + reflectivity.z;
	float sumRefract = m_IOR > 0.0f ? refractivity.x + refractivity.y + refractivity.z : 0.0f;
	float sumInv = 1.0f / (sumDiffuse + sumSpecular + sumReflect + sumRefract);

	if (!isnan(sumInv))
	{
		m_sampleProbs.Diffuse = sumDiffuse * sumInv;
		m_sampleProbs.Specular = sumSpecular * sumInv;
		m_sampleProbs.Reflect = sumReflect * sumInv;
		m_sampleProbs.Refract = sumRefract * sumInv;
	}
	else
	{
		m_sampleProbs.Diffuse = 0.0f;
		m_sampleProbs.Specular = 0.0f;
		m_sampleProbs.Reflect = 0.0f;
		m_sampleProbs.Refract = 0.0f;
	}
}
