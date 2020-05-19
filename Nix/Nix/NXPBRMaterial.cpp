#include "NXPBRMaterial.h"
#include "NXReflectionModel.h"
#include "NXBSDF.h"

void NXMatteMaterial::ConstructReflectionModel(NXHit& hitInfo)
{
	hitInfo.BSDF = make_shared<NXBSDF>(hitInfo, IOR);
	if (Diffuse != Vector3(0.0f))
	{
		hitInfo.BSDF->AddReflectionModel(make_shared<NXRLambertianReflection>(Diffuse));
	}
}
