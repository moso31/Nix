#pragma once
#include "NXInstance.h"
#include "NXBSDF.h"

#define NXRT_EPSILON 1e-4f	// NXRayTracer 浮点精度校正

struct NXShadingHit
{
	Vector3 normal;
	Vector3 dpdu, dpdv;
};

class NXHit
{
public:
	NXHit() : faceIndex(-1) {}
	NXHit(NXPrimitive* pPrimitive, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv);
	~NXHit() {}

	void GenerateBSDF(bool IsFromCamera);
	void SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv);

	// 计算完毕后得到的hitInfo是Local空间数据。使用本方法将其数据转换到world空间。
	void LocalToWorld();

	// 重设primitive
	void Reset();

	NXPrimitive* pPrimitive;

	Vector3 direction;	// 入射方向
	Vector3 position;
	Vector3 normal;	// 几何法线 geometry normal
	Vector2 uv;
	Vector3 dpdu;
	Vector3 dpdv;
	NXShadingHit shading;

	NXBSDF* BSDF;

	int faceIndex;
};
