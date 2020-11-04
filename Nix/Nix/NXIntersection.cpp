#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXPBRMaterial.h"

NXHit::NXHit(NXPrimitive* pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv) :
	pPrimitive(pPrimitive),
	position(position),
	uv(uv),
	direction(direction),
	dpdu(dpdu),
	dpdv(dpdv),
	normal(dpdv.Cross(dpdu)),
	faceIndex(-1),
	BSDF(nullptr)
{
	normal.Normalize();
}

NXHit::~NXHit()
{
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;
	}
}

void NXHit::GenerateBSDF(bool IsFromCamera)
{
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;
	}
	NXPBRMaterial* pMat = pPrimitive->GetPBRMaterial();
	BSDF = new NXBSDF(*this, pMat);
}

void NXHit::SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv)
{
	shading.dpdu = shadingdpdu;
	shading.dpdv = shadingdpdv;
	shading.normal = shadingdpdv.Cross(shadingdpdu);

	// 按理说击中点处的主法向量和shading法向量应该始终处于同一方向
	// 但由于主法向量一定是使用dpdu和dpdv的叉积计算的，而shading法向量则不一定（还可能使用mesh本身数据）
	// 所以基于以mesh本身数据为准的原则，应该让主normal遵循shading.normal的朝向。
	if (normal.Dot(shading.normal) < 0)
		normal = -normal;
}

void NXHit::LocalToWorld()
{
	Matrix mxWorld = pPrimitive->GetWorldMatrix();
	position = Vector3::Transform(position, mxWorld);
	Vector3::TransformNormal(direction, mxWorld).Normalize(direction);
	Vector3::TransformNormal(normal, mxWorld).Normalize(normal);
	Vector3::TransformNormal(dpdu, mxWorld).Normalize(dpdu);
	Vector3::TransformNormal(dpdv, mxWorld).Normalize(dpdv);
	Vector3::TransformNormal(shading.normal, mxWorld).Normalize(shading.normal);
	Vector3::TransformNormal(shading.dpdu, mxWorld).Normalize(shading.dpdu);
	Vector3::TransformNormal(shading.dpdv, mxWorld).Normalize(shading.dpdv);
}

void NXHit::Reset()
{
	pPrimitive = nullptr;
	if (BSDF)
	{
		BSDF->Release();
		delete BSDF;

		// Reset以后BSDF指针是有可能重用的，所以需要将BSDF指针置空，以确保下次使用时的内存安全。
		BSDF = nullptr;	
	}
}
