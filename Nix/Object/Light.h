#pragma once
#include "Header.h"

struct LightInfo
{
	LightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 direction;
};

class Light
{
public:
	Light() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	Vector3 GetDirection();
	LightInfo GetLightInfo();

	void SetAmbient(Vector4 ambient);
	void SetDiffuse(Vector4 diffuse);
	void SetSpecular(Vector4 specular);
	void SetDirection(Vector3 direction);

	void Update();
	void Render();

private:
	Vector4 m_ambient;
	Vector4 m_diffuse;
	Vector4 m_specular;
	Vector3 m_direction;
};