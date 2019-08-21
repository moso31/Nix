#pragma once
#include "Header.h"

struct DirectionalLightInfo
{
	DirectionalLightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 direction;
};

struct PointLightInfo
{
	PointLightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 direction;
};

class DirectionalLight
{
public:
	DirectionalLight() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	Vector3 GetDirection();
	DirectionalLightInfo GetLightInfo();

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

class PointLight
{
public:
	PointLight() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	Vector3 GetDirection();
	PointLightInfo GetLightInfo();

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