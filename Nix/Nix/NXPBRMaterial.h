#pragma once
#include "NXIntersection.h"

// дһ��PBR���ʡ�������Ⱦ��ʹ��PBR���ʡ�
// �����Ὣ��ͨ���ʺ�PBR���ʺϲ���
class NXPBRMaterial
{
public:
	NXPBRMaterial() {}
	~NXPBRMaterial() {}

	virtual void ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo) = 0;

protected:
	Vector3 m_diffuse;
	Vector3 m_specular;
	float m_roughness;
	Vector3 m_reflectivity;
	float m_IOR;
};

class NXMatteMaterial : public NXPBRMaterial
{
public:
	void ConstructReflectionModel(const shared_ptr<NXHit>& hitInfo);

private:

};