#pragma once
#include "Header.h"
#include "NXInstance.h"

struct NXHitInfo
{
	NXHitInfo() : primitive(nullptr), distance(0) {}
	// 击中物体
	shared_ptr<NXPrimitive> primitive;
	Vector3 position;
	float distance;
};

class NXHit : public NXInstance<NXHit>
{
public:
	NXHit();
	~NXHit();

	// 提供场景和射线数据（世界坐标），计算交点及相关信息。
	bool RayCast(const shared_ptr<NXScene>& pScene, const Ray& ray, NXHitInfo& oInfo);

private:
	
};
