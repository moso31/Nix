#pragma once
#include "NXIntersection.h"

// дһ��PBR���ʡ�������Ⱦ��ʹ��PBR���ʡ�
// �����Ὣ��ͨ���ʺ�PBR���ʺϲ���
class NXPBRMaterial
{
public:
	NXPBRMaterial() {}
	~NXPBRMaterial() {}

	virtual void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) = 0;

	Vector3 Diffuse;
	Vector3 Specular;
	float Roughness;
	float Metalness;
	float IOR;
};

class NXMatteMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) override;
};

class NXMirrorMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) override;
};

class NXGlassMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) override;
};

class NXPlasticMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) override;
};

// ���������Լ������һ��ͨ�õĲ����ࡣ
// û�п������䡣
class NXCommonMaterial : public NXPBRMaterial
{
public:
	NXCommonMaterial(const Vector3& BaseColor, float Metalness, float Roughness) : BaseColor(BaseColor) { this->Metalness = Metalness; this->Roughness = Roughness; }
	~NXCommonMaterial() {}

	void ConstructReflectionModel(NXHit& hitInfo, bool IsFromCamera) override;

	Vector3 BaseColor;
};
