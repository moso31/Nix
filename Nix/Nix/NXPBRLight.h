#pragma once
#include "NXInstance.h"
#include "ShaderStructures.h"
#include <string>

enum NXLightTypeEnum
{
	NXLight_Unknown,
	NXLight_Distant,
	NXLight_Point,
	NXLight_Spot,
};

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

class NXPBRDistantLight : public NXPBRLight
{
public:
	NXPBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance);

	// DirectX
	ConstantBufferDistantLight GetConstantBuffer();

	Vector3		GetDirection() { return m_direction; }
	Vector3		GetColor() { return m_color; }
	float		GetIlluminance() { return m_illuminance; }

	void		SetDirection(const Vector3& direction) { m_direction = direction; }
	void		SetColor(const Vector3& color) { m_color = color; }
	void		SetIlluminance(const float illuminance) { m_illuminance = illuminance; }

private:
	Vector3		m_direction;
	Vector3		m_color;
	float		m_illuminance;		// unit: lux, lm/m^2
};

// ��ʱPBR��Դ
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius);
	virtual ~NXPBRPointLight() {}

	// DirectX
	ConstantBufferPointLight GetConstantBuffer();

	Vector3		GetPosition() { return m_position; }
	Vector3		GetColor() { return m_color; }
	float		GetIntensity() { return m_intensity; }
	float		GetInfluenceradius() { return m_influenceRadius; }

	void		SetPosition(const Vector3& position) { m_position = position; }
	void		SetColor(const Vector3& color) { m_color = color; }
	void		SetIntensity(const float intensity) { m_intensity = intensity; }
	void		SetInfluenceRadius(const float influenceRadius) { m_influenceRadius = influenceRadius; }

private:
	Vector3		m_position;
	Vector3		m_color;
	float		m_intensity;		// unit: lm
	float		m_influenceRadius;
};

class NXPBRSpotLight : public NXPBRLight
{
public:
	NXPBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius);

	// DirectX
	ConstantBufferSpotLight GetConstantBuffer();

	Vector3		GetPosition() { return m_position; }
	Vector3		GetDirection() { return m_direction; }
	Vector3		GetColor() { return m_color; }
	float		GetIntensity() { return m_intensity; }
	float		GetInnerAngle() { return m_innerAngle; }
	float		GetOuterAngle() { return m_outerAngle; }
	float		GetInfluenceradius() { return m_influenceRadius; }

	void		SetPosition(const Vector3& position) { m_position = position; }
	void		SetDirection(const Vector3& direction) { m_direction = direction; }
	void		SetColor(const Vector3& color) { m_color = color; }
	void		SetIntensity(const float intensity) { m_intensity = intensity; }
	void		SetInnerAngle(const float value) { m_innerAngle = value; }
	void		SetOuterAngle(const float value) { m_outerAngle = value; }
	void		SetInfluenceRadius(const float influenceRadius) { m_influenceRadius = influenceRadius; }

private:
	Vector3		m_position;
	Vector3		m_direction;
	float		m_innerAngle;
	float		m_outerAngle;
	Vector3		m_color;
	float		m_intensity;		// unit: lm
	float		m_influenceRadius;
};
