#pragma once
#include "NXIntersection.h"

// 写一个PBR材质。离线渲染先使用PBR材质。
// 将来会将普通材质和PBR材质合并。
class NXPBRMaterial
{
public:
	NXPBRMaterial() {}
	~NXPBRMaterial() {}

	virtual void ConstructReflectionModel(NXHit& hitInfo) = 0;

	Vector3 Diffuse;
	Vector3 Specular;
	float Roughness;
	float Metalness;
	float IOR;
};

class NXMatteMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo) override;
};

class NXMirrorMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo) override;
};

class NXGlassMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo) override;
};

class NXPlasticMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo) override;
};

// 尝试重新自己设计了一个通用的材质类。
// 没有考虑折射。
class NXCommonMaterial : public NXPBRMaterial
{
public:
	NXCommonMaterial(const Vector3& BaseColor, float Metalness, float Roughness) : BaseColor(BaseColor) { this->Metalness = Metalness; this->Roughness = Roughness; }
	~NXCommonMaterial() {}

	void ConstructReflectionModel(NXHit& hitInfo) override;

	Vector3 BaseColor;
};
