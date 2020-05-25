#pragma once
#include "NXIntersection.h"

// дһ��PBR���ʡ�������Ⱦ��ʹ��PBR���ʡ�
// �����Ὣ��ͨ���ʺ�PBR���ʺϲ���
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

// ���������Լ������һ��ͨ�õĲ����ࡣ
// û�п������䡣
class NXCommonMaterial : public NXPBRMaterial
{
public:
	NXCommonMaterial(const Vector3& BaseColor, float Metalness, float Roughness) : BaseColor(BaseColor) { this->Metalness = Metalness; this->Roughness = Roughness; }
	~NXCommonMaterial() {}

	void ConstructReflectionModel(NXHit& hitInfo) override;

	Vector3 BaseColor;
};
