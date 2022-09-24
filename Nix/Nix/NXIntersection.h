#pragma once
#include "Header.h"

class NXHit
{
public:
	NXHit() : faceIndex(-1) {}
	NXHit(NXSubMeshBase* pSubMesh, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv);
	~NXHit() {}

	// ������Ϻ�õ���hitInfo��Local�ռ����ݡ�ʹ�ñ�������������ת����world�ռ䡣
	void LocalToWorld();

	// ����primitive
	void Reset();

	NXSubMeshBase* pSubMesh;

	Vector3 direction;	// ���䷽��
	Vector3 position;
	Vector3 normal;	// ���η��� geometry normal
	Vector2 uv;
	Vector3 dpdu;
	Vector3 dpdv;

	int faceIndex;
};
