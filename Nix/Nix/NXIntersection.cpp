#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPBRMaterial.h"

#include "NXPrimitive.h"

NXHit::NXHit(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& dpdu, const Vector3& dpdv) :
	pPrimitive(pPrimitive),
	position(position),
	uv(uv),
	dpdu(dpdu),
	dpdv(dpdv),
	normal(dpdu.Cross(dpdv))
{
}

void NXHit::ConstructReflectionModel()
{
	shared_ptr<NXPBRMaterial> pMat = pPrimitive->GetPBRMaterial();
	if (pMat)
		pMat->ConstructReflectionModel(shared_from_this());
}

void NXHit::SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv)
{
	shading.dpdu = shadingdpdu;
	shading.dpdv = shadingdpdv;
	shading.normal = shadingdpdu.Cross(shadingdpdv);

	// 按理说击中点处的主法向量和shading法向量应该始终处于同一方向
	// 但由于主法向量一定是使用dpdu和dpdv的叉积计算的，而shading法向量则不一定（还可能使用mesh本身数据）
	// 所以基于以mesh本身数据为准的原则，应该让主normal遵循shading.normal的朝向。
	if (normal.Dot(shading.normal) < 0)
		normal = -normal;
}
