#pragma once
#include "ShaderStructures.h"

class NXMaterial
{
public:
	NXMaterial() {}
	~NXMaterial() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	Vector4 GetReflect();
	float GetOpacity();
	ConstantBufferMaterial GetMaterialInfo();

	void SetAmbient(Vector4 ambient);
	void SetDiffuse(Vector4 diffuse);
	void SetSpecular(Vector4 specular);
	void SetReflect(Vector4 reflect);
	void SetOpacity(float opacity);

	void Update();
	void Render();

private:
	Vector4 m_ambient;
	Vector4 m_diffuse;
	Vector4 m_specular; // w = SpecPower
	Vector4 m_reflect;
	float m_opacity;
};

// дһ��PBR���ʡ�������Ⱦ��ʹ��PBR���ʡ�
// �����Ὣ��ͨ���ʺ�PBR���ʺϲ���
class NXPBRMaterial
{
public:
	NXPBRMaterial() {}
	~NXPBRMaterial() {}

private:
	Vector3 m_diffuse;
	Vector3 m_specular;
	float m_roughness;
	Vector3 m_reflectivity;
	float m_IOR;
};