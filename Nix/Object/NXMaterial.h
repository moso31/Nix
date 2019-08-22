#pragma once
#include "Header.h"

struct MaterialInfo
{
	MaterialInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular; // w = SpecPower
	Vector4 reflect;
};

class NXMaterial
{
public:
	NXMaterial() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	Vector4 GetReflect();
	MaterialInfo GetMaterialInfo();

	void SetAmbient(Vector4 ambient);
	void SetDiffuse(Vector4 diffuse);
	void SetSpecular(Vector4 specular);
	void SetReflect(Vector4 reflect);

	void Update();
	void Render();

private:
	Vector4 m_ambient;
	Vector4 m_diffuse;
	Vector4 m_specular; // w = SpecPower
	Vector4 m_reflect;
};