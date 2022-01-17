#pragma once
#include "NXInstance.h"
#include "ShaderStructures.h"

class NXPBRLight
{
public:
	NXPBRLight() {}
	virtual ~NXPBRLight() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

public:
	std::string m_name;
};

// ¡Ÿ ±PBRπ‚‘¥
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& position, const Vector3& intensity);
	~NXPBRPointLight() {}

	// DirectX
	ConstantBufferPointLight GetConstantBuffer();

private:
	Vector3 m_position;
	Vector3 m_intensity;
};

class NXPBRDistantLight : public NXPBRLight
{
public:
	NXPBRDistantLight(const Vector3& direction, const Vector3& radiance, Vector3 worldCenter, float worldRadius);

	// DirectX
	ConstantBufferDistantLight GetConstantBuffer();

private:
	Vector3 m_direction;
	Vector3 m_radiance;
	Vector3 m_worldCenter;
	float m_worldRadius;
};

class NXPBREnvironmentLight : public NXPBRLight
{
public:
	NXPBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& radiance, Vector3 worldCenter, float worldRadius);

private:
	Vector3 m_radiance;
	Vector3 m_worldCenter;
	float m_worldRadius;

private:
	NXCubeMap* m_pCubeMap;
};