#include "NXPBRMaterial.h"
#include "NXReflectionModel.h"
#include "NXBSDF.h"

void NXMatteMaterial::ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo)
{
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo->BSDF->AddReflectionModel(make_shared<NXRLambertianReflection>(Diffuse));
	}
}
