#pragma once
#include "Header.h"
#include "NXInstance.h"

struct NXIntersectionInfo
{
	NXIntersectionInfo() : primitive(nullptr), distance(0) {}
	// 击中物体
	shared_ptr<NXPrimitive> primitive;
	Vector3 position;
	float distance;
};

class NXIntersection : public NXInstance<NXIntersection>
{
public:
	NXIntersection();
	~NXIntersection();

	// 提供场景和射线数据（世界坐标），计算交点及相关信息。
	bool RayIntersect(const shared_ptr<NXScene>& pScene, const Ray& ray, NXIntersectionInfo& oInfo);

private:
	
};
