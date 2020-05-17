#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPBRMaterial.h"

#include "NXPrimitive.h"

void NXHit::ConstructReflectionModel()
{
	shared_ptr<NXPBRMaterial> pMat = primitive->GetPBRMaterial();
	if (pMat)
		pMat->ConstructReflectionModel(shared_from_this());
}
