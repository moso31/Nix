#pragma once
#include "NXInstance.h"
#include "NXBSDF.h"

#define NXRT_EPSILON 1e-4f	// NXRayTracer ���㾫��У��

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

	void ConstructReflectionModel(bool IsFromCamera);
	void SetShadingGeometry(Vector3 shadingdpdu, Vector3 shadingdpdv);
	
	// ������Ϻ�õ���hitInfo��Local�ռ����ݡ�ʹ�ñ�������������ת����world�ռ䡣
	void LocalToWorld();

	shared_ptr<NXPrimitive> pPrimitive;

	Vector3 direction;	// ���䷽��
	Vector3 position;
	Vector3 normal;	// ���η��� geometry normal
	Vector2 uv;
	Vector3 dpdu;
	Vector3 dpdv;
	NXShadingHit shading;

	shared_ptr<NXBSDF> BSDF;

	int faceIndex;
};
