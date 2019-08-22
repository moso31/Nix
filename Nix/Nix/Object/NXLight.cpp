#include "NXLight.h"

Vector4 NXDirectionalLight::GetAmbient()
{
	return m_ambient;
}

Vector4 NXDirectionalLight::GetDiffuse()
{
	return m_diffuse;
}

Vector4 NXDirectionalLight::GetSpecular()
{
	return m_specular;
}

Vector3 NXDirectionalLight::GetDirection()
{
	return m_direction;
}

DirectionalLightInfo NXDirectionalLight::GetLightInfo()
{
	DirectionalLightInfo li;
	li.ambient = m_ambient;
	li.diffuse = m_diffuse;
	li.direction = m_direction;
	li.specular = m_specular;
	return li;
}

void NXDirectionalLight::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void NXDirectionalLight::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void NXDirectionalLight::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void NXDirectionalLight::SetDirection(Vector3 direction)
{
	m_direction = direction;
	m_direction.Normalize();
}

void NXDirectionalLight::Update()
{
}

void NXDirectionalLight::Render()
{
}

Vector4 NXPointLight::GetAmbient()
{
	return m_ambient;
}

Vector4 NXPointLight::GetDiffuse()
{
	return m_diffuse;
}

Vector4 NXPointLight::GetSpecular()
{
	return m_specular;
}

Vector3 NXPointLight::GetPosition()
{
	return m_position;
}

float NXPointLight::GetRange()
{
	return m_range;
}

Vector3 NXPointLight::GetAtt()
{
	return m_att;
}

PointLightInfo NXPointLight::GetLightInfo()
{
	PointLightInfo li;
	li.ambient = m_ambient;
	li.diffuse = m_diffuse;
	li.specular = m_specular;
	li.position = m_position;
	li.range = m_range;
	li.att = m_att;
	return li;
}

void NXPointLight::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void NXPointLight::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void NXPointLight::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void NXPointLight::SetPosition(Vector3 position)
{
	m_position = position;
}

void NXPointLight::SetRange(float range)
{
	m_range = range;
}

void NXPointLight::SetAtt(Vector3 att)
{
	m_att = att;
}

void NXPointLight::Update()
{
}

void NXPointLight::Render()
{
}

Vector4 NXSpotLight::GetAmbient()
{
	return m_ambient;
}

Vector4 NXSpotLight::GetDiffuse()
{
	return m_diffuse;
}

Vector4 NXSpotLight::GetSpecular()
{
	return m_specular;
}

Vector3 NXSpotLight::GetPosition()
{
	return m_position;
}

float NXSpotLight::GetRange()
{
	return m_range;
}

Vector3 NXSpotLight::GetDirection()
{
	return m_direction;
}

float NXSpotLight::GetSpot()
{
	return m_spot;
}

Vector3 NXSpotLight::GetAtt()
{
	return m_att;
}

SpotLightInfo NXSpotLight::GetLightInfo()
{
	SpotLightInfo li;
	li.ambient = m_ambient;
	li.diffuse = m_diffuse;
	li.specular = m_specular;
	li.position = m_position;
	li.range = m_range;
	li.direction = m_direction;
	li.spot = m_spot;
	li.att = m_att;
	return li;
}

void NXSpotLight::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void NXSpotLight::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void NXSpotLight::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void NXSpotLight::SetPosition(Vector3 position)
{
	m_position = position;
}

void NXSpotLight::SetRange(float range)
{
	m_range = range;
}

void NXSpotLight::SetDirection(Vector3 direction)
{
	m_direction = direction;
	m_direction.Normalize();
}

void NXSpotLight::SetSpot(float spot)
{
	m_spot = spot;
}

void NXSpotLight::SetAtt(Vector3 att)
{
	m_att = att;
}

void NXSpotLight::Update()
{
}

void NXSpotLight::Render()
{
}
