#pragma once
#include "Header.h"

class NXHit
{
public:
	NXHit() : faceIndex(-1) {}
	NXHit(NXSubMeshBase* pSubMesh, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv);
	~NXHit() {}

	// 计算完毕后得到的hitInfo是Local空间数据。使用本方法将其数据转换到world空间。
	void LocalToWorld();

	// 重设primitive
	void Reset();

	NXSubMeshBase* pSubMesh;

	Vector3 direction;	// 入射方向
	Vector3 position;
	Vector3 normal;	// 几何法线 geometry normal
	Vector2 uv;
	Vector3 dpdu;
	Vector3 dpdv;

	int faceIndex;
};
