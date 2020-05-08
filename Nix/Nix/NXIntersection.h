#pragma once
#include "Header.h"
#include "NXInstance.h"

struct NXIntersectionInfo
{
	NXIntersectionInfo() : primitive(nullptr), distance(0) {}
	// ��������
	shared_ptr<NXPrimitive> primitive;
	Vector3 position;
	float distance;
};

class NXIntersection : public NXInstance<NXIntersection>
{
public:
	NXIntersection();
	~NXIntersection();

	// �ṩ�������������ݣ��������꣩�����㽻�㼰�����Ϣ��
	bool RayIntersect(const shared_ptr<NXScene>& pScene, const Ray& ray, NXIntersectionInfo& oInfo);

private:
	
};
