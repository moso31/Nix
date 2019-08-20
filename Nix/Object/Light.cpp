#include "Light.h"

Vector4 Light::GetAmbient()
{
	return m_ambient;
}

Vector4 Light::GetDiffuse()
{
	return m_diffuse;
}

Vector4 Light::GetSpecular()
{
	return m_specular;
}

Vector3 Light::GetDirection()
{
	return m_direction;
}

LightInfo Light::GetLightInfo()
{
	LightInfo li;
	li.ambient = m_ambient;
	li.diffuse = m_diffuse;
	li.direction = m_direction;
	li.specular = m_specular;
	return li;
}

void Light::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void Light::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void Light::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void Light::SetDirection(Vector3 direction)
{
	m_direction = direction;
	m_direction.Normalize();
}

void Light::Update()
{
}

void Light::Render()
{
}
