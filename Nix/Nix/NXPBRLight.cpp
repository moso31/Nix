#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXRandom.h"

using namespace SamplerMath;

NXPBRPointLight::NXPBRPointLight(const Vector3& position, const Vector3& intensity) :
	m_position(position), 
	m_intensity(intensity)
{
	m_name = "Point Light";
}

ConstantBufferPointLight NXPBRPointLight::GetConstantBuffer()
{
	ConstantBufferPointLight cb;
	cb.position = m_position;
	cb._0 = 0;
	cb.color = m_intensity;
	cb._1 = 0;
	return cb;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& direction, const Vector3& radiance, Vector3 worldCenter, float worldRadius) :
	m_direction(direction), 
	m_radiance(radiance),
	m_worldCenter(worldCenter),
	m_worldRadius(worldRadius)
{
	this->m_direction.Normalize();
	m_name = "Distant Light";
}

ConstantBufferDistantLight NXPBRDistantLight::GetConstantBuffer()
{
	ConstantBufferDistantLight cb;
	cb.direction = m_direction;
	cb.color = m_radiance;
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
