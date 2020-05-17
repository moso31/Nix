#pragma once
#include "NXTransform.h"

struct DirectionalLightInfo
{
	DirectionalLightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 direction;
	float _align16;
};

struct PointLightInfo
{
	PointLightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 position;
	float range;
	Vector3 att;
	float _align16;
};

struct SpotLightInfo
{
	SpotLightInfo() {}

	Vector4 ambient;
	Vector4 diffuse;
	Vector4 specular;
	Vector3 position;
	float range;
	Vector3 direction;
	float spot;
	Vector3 att;
	float _align16;
};

class NXLight : public NXTransform
{
public:
	NXLight() {}
};

class NXDirectionalLight : public NXLight
{
public:
	NXDirectionalLight() {}

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

class NXPointLight : public NXLight
{
public:
	NXPointLight() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	float GetRange();
	Vector3 GetAtt();
	PointLightInfo GetLightInfo();

	void SetAmbient(Vector4 ambient);
	void SetDiffuse(Vector4 diffuse);
	void SetSpecular(Vector4 specular);
	void SetRange(float range);
	void SetAtt(Vector3 att);

	void Update();
	void Render();

private:
	Vector4 m_ambient;
	Vector4 m_diffuse;
	Vector4 m_specular;
	float m_range;
	Vector3 m_att;
};

class NXSpotLight : public NXLight
{
public:
	NXSpotLight() {}

	Vector4 GetAmbient();
	Vector4 GetDiffuse();
	Vector4 GetSpecular();
	float GetRange();
	Vector3 GetDirection();
	float GetSpot();
	Vector3 GetAtt();
	SpotLightInfo GetLightInfo();

	void SetAmbient(Vector4 ambient);
	void SetDiffuse(Vector4 diffuse);
	void SetSpecular(Vector4 specular);
	void SetRange(float range);
	void SetDirection(Vector3 direction);
	void SetSpot(float spot);
	void SetAtt(Vector3 att);

	void Update();
	void Render();

private:
	Vector4 m_ambient;
	Vector4 m_diffuse;
	Vector4 m_specular;
	float m_range;
	Vector3 m_direction;
	float m_spot;
	Vector3 m_att;
};
