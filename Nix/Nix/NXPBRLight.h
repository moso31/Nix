#pragma once
#include "NXInstance.h"
#include "ShaderStructures.h"

enum NXLightTypeEnum
{
	NXLight_Unknown,
	NXLight_Distant,
	NXLight_Point,
	NXLight_Spot,
};

//const char* NXLightTypeEnumStr[] =
//{
//	"NXLight_Unknown",
//	"NXLight_Distant",
//	"NXLight_Point",
//	"NXLight_Spot"
//};

class NXPBRLight
{
public:
	NXPBRLight() { m_name = "Unknown Light"; m_type = NXLight_Unknown; }
	virtual ~NXPBRLight() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	NXLightTypeEnum GetType() { return m_type; }
	//std::string GetTypeString() { return NXLightTypeEnumStr[m_type]; }

public:
	std::string m_name;
	NXLightTypeEnum m_type;
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
	NXPBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance);

	// DirectX
	ConstantBufferDistantLight GetConstantBuffer();

	Vector3 GetDirection()		{ return m_direction; }
	Vector3 GetColor()			{ return m_color; }
	float	GetIlluminance()	{ return m_illuminance; }

	void	SetDirection(const Vector3& direction)		{ m_direction = direction; }
	void	SetColor(const Vector3& color)				{ m_color = color; }
	void	SetIlluminance(const float illuminance)		{ m_illuminance = illuminance; }

private:
	Vector3 m_direction;
	Vector3 m_color;	
	float	m_illuminance;		// unit: lux, lm/m^2
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