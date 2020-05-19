#pragma once
#include "NXIntersection.h"

// 写一个PBR材质。离线渲染先使用PBR材质。
// 将来会将普通材质和PBR材质合并。
class NXPBRMaterial
{
public:
	NXPBRMaterial() {}
	~NXPBRMaterial() {}

	virtual void ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo) = 0;

	Vector3 Diffuse;
	Vector3 m_specular;
	float m_roughness;
	Vector3 m_reflectivity;
	float IOR;
};

class NXMatteMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo);

private:

};