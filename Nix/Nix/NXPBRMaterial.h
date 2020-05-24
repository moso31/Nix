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
	Vector3 m_reflectivity;
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

class NXMetalMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(NXHit& hitInfo) override;
};
