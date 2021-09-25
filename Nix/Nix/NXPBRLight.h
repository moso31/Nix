#pragma once
#include "NXInstance.h"
#include "ShaderStructures.h"

class NXPBRLight
{
public:
	NXPBRLight() {}
	virtual ~NXPBRLight() {}
};

// ¡Ÿ ±PBRπ‚‘¥
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	// DirectX
	ConstantBufferPointLight GetConstantBuffer();

public:
	Vector3 Position;
	Vector3 Intensity;
};

class NXPBRDistantLight : public NXPBRLight
{
public:
	NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius);

	// DirectX
	ConstantBufferDistantLight GetConstantBuffer();

public:
	Vector3 Direction;
	Vector3 Radiance;
	Vector3 WorldCenter;
	float WorldRadius;
};

class NXPBREnvironmentLight : public NXPBRLight
{
public:
	NXPBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius);

public:
	Vector3 Radiance;
	Vector3 WorldCenter;
	float WorldRadius;

private:
	NXCubeMap* m_pCubeMap;
};