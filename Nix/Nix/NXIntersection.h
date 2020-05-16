#pragma once
#include "Header.h"
#include "NXInstance.h"

struct NXHitInfo
{
	NXHitInfo() : primitive(nullptr), distance(0) {}
	// ��������
	shared_ptr<NXPrimitive> primitive;
	Vector3 position;
	float distance;
};

class NXHit : public NXInstance<NXHit>
{
public:
	NXHit();
	~NXHit();

	// �ṩ�������������ݣ��������꣩�����㽻�㼰�����Ϣ��
	bool RayCast(const shared_ptr<NXScene>& pScene, const Ray& ray, NXHitInfo& oInfo);

private:
	
};
