#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXRandom.h"

using namespace SamplerMath;

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

NXPBRPointLight::NXPBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius) :
	m_position(position), 
	m_color(color),
	m_intensity(intensity),
	m_influenceRadius(influenceRadius)
{
	m_name = "Point Light";
	m_type = NXLight_Point;
}

ConstantBufferPointLight NXPBRPointLight::GetConstantBuffer()
{
	ConstantBufferPointLight cb;
	cb.position = m_position;
	cb.influenceRadius = m_influenceRadius;
	cb.color = m_color;
	cb.intensity = m_intensity;
	return cb;
}

NXPBRSpotLight::NXPBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius) :
	m_position(position),
	m_direction(direction),
	m_color(color),
	m_intensity(intensity),
	m_innerAngle(innerAngle),
	m_outerAngle(outerAngle),
	m_influenceRadius(influenceRadius)
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
	cb.influenceRadius = m_influenceRadius;
	return cb;
}
