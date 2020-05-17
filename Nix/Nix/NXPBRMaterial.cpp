#include "NXPBRMaterial.h"
#include "NXReflectionModel.h"
#include "NXBSDF.h"

void NXMatteMaterial::ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo)
{
	if (m_diffuse != Vector3(0.0f))
	{
		hitInfo->bsdf.AddReflectionModel(make_shared<NXRLambertianReflection>(m_diffuse));
	}
}
