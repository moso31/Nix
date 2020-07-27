#include "NXReflectionModel.h"
#include "NXPBRMaterial.h"

NXPBRMaterial::NXPBRMaterial(const Vector3& Diffuse, const Vector3& Specular, const Vector3& Reflectivity, float Roughness, float IOR) :
	m_diffuse(Diffuse),
	m_specular(Specular),
	m_reflectivity(Reflectivity),
	m_roughness(Roughness),
	m_IOR(IOR)
{
	CalcSampleProbabilities();
}

void NXPBRMaterial::CalcSampleProbabilities()
{
	float prob = (m_diffuse + m_specular).MaxComponent();
	m_probability = Clamp(prob, 0.0f, 1.0f);

	float sumDiffuse = m_diffuse.x + m_diffuse.y + m_diffuse.z;
	float sumSpecular = m_specular.x + m_specular.y + m_specular.z;
	float sumInv = 1.0f / (sumDiffuse + sumSpecular);

	if (!isnan(sumInv))
	{
		m_sampleProbs.Diff = sumDiffuse * sumInv;
		m_sampleProbs.Spec = sumSpecular * sumInv;
	}
	else
	{
		m_sampleProbs.Diff = 0.0f;
		m_sampleProbs.Spec = 0.0f;
	}
}
