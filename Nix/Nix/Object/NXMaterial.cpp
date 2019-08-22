#include "NXMaterial.h"

Vector4 NXMaterial::GetAmbient()
{
	return m_ambient;
}

Vector4 NXMaterial::GetDiffuse()
{
	return m_diffuse;
}

Vector4 NXMaterial::GetSpecular()
{
	return m_specular;
}

Vector4 NXMaterial::GetReflect()
{
	return m_reflect;
}

MaterialInfo NXMaterial::GetMaterialInfo()
{
	MaterialInfo mi;
	mi.ambient = m_ambient;
	mi.diffuse = m_diffuse;
	mi.specular = m_specular;
	mi.reflect = m_reflect;
	return mi;
}

void NXMaterial::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void NXMaterial::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void NXMaterial::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void NXMaterial::SetReflect(Vector4 Reflect)
{
	m_reflect = Reflect;
}

void NXMaterial::Update()
{
}

void NXMaterial::Render()
{
}
