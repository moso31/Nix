#pragma once
#include "NXInstance.h"
#include "NXBSDF.h"

struct NXShadingHit
{
	Vector3 normal;
	Vector3 dpdu, dpdv;
};

class NXHit : public enable_shared_from_this<NXHit>
{
public:
	NXHit() : faceIndex(-1) {}
	NXHit(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv);
	~NXHit() {}

	void ConstructReflectionModel();
	void SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv);
	
	// 计算完毕后得到的hitInfo是Local空间数据。使用本方法将其数据转换到world空间。
	void LocalToWorld();

	shared_ptr<NXPrimitive> pPrimitive;

	Vector3 direction;	// 入射方向
	Vector3 position;
	Vector3 normal;	// 几何法线 geometry normal
	Vector2 uv;
	Vector3 dpdu;
	Vector3 dpdv;
	NXShadingHit shading;

	shared_ptr<NXBSDF> BSDF;

	int faceIndex;
};
