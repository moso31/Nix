#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXRandom.h"

using namespace SamplerMath;

NXPBRPointLight::NXPBRPointLight(const Vector3& position, const Vector3& color, const float intensity) :
	m_position(position), 
	m_color(color),
	m_intensity(intensity)
{
	m_name = "Point Light";
	m_type = NXLight_Point;
}

ConstantBufferPointLight NXPBRPointLight::GetConstantBuffer()
{
	ConstantBufferPointLight cb;
	cb.position = m_position;
	cb._0 = 0;
	cb.color = m_color;
	cb.intensity = m_intensity;
	return cb;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance) :
	m_direction(direction), 
	m_color(color),
	m_illuminance(illuminance)
{
	this->m_direction.Normalize();
	m_name = "Distant Light";
	m_type = NXLight_Distant;
}

ConstantBufferDistantLight NXPBRDistantLight::GetConstantBuffer()
{
	ConstantBufferDistantLight cb;
	cb.direction = m_direction;
	cb.color = m_color;
	cb.illuminance = m_illuminance;
	return cb;
}

NXPBRSpotLight::NXPBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle) :
	m_position(position),
	m_direction(direction),
	m_color(color),
	m_intensity(intensity),
	m_innerAngle(innerAngle),
	m_outerAngle(outerAngle)
{
	m_name = "Spot Light";
	m_type = NXLight_Spot;
}

ConstantBufferSpotLight NXPBRSpotLight::GetConstantBuffer()
{
	ConstantBufferSpotLight cb;
	cb.position = m_position;
	cb.direction = m_direction;
	cb.color = m_color;
	cb.intensity = m_intensity;
	cb.innerAngle = m_innerAngle;
	cb.outerAngle = m_outerAngle;
	return cb;
}

NXPBREnvironmentLight::NXPBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& radiance, Vector3 worldCenter, float worldRadius) :
	m_pCubeMap(pCubeMap),
	m_radiance(radiance),
	m_worldCenter(worldCenter),
	m_worldRadius(worldRadius)
{
	m_name = "Environment Light";
}
