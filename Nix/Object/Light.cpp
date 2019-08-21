#include "Light.h"

Vector4 DirectionalLight::GetAmbient()
{
	return m_ambient;
}

Vector4 DirectionalLight::GetDiffuse()
{
	return m_diffuse;
}

Vector4 DirectionalLight::GetSpecular()
{
	return m_specular;
}

Vector3 DirectionalLight::GetDirection()
{
	return m_direction;
}

DirectionalLightInfo DirectionalLight::GetLightInfo()
{
	DirectionalLightInfo li;
	li.ambient = m_ambient;
	li.diffuse = m_diffuse;
	li.direction = m_direction;
	li.specular = m_specular;
	return li;
}

void DirectionalLight::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void DirectionalLight::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void DirectionalLight::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void DirectionalLight::SetDirection(Vector3 direction)
{
	m_direction = direction;
	m_direction.Normalize();
}

void DirectionalLight::Update()
{
}

void DirectionalLight::Render()
{
}
