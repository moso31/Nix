#include "Material.h"

Vector4 Material::GetAmbient()
{
	return m_ambient;
}

Vector4 Material::GetDiffuse()
{
	return m_diffuse;
}

Vector4 Material::GetSpecular()
{
	return m_specular;
}

Vector4 Material::GetReflect()
{
	return m_reflect;
}

MaterialInfo Material::GetMaterialInfo()
{
	MaterialInfo mi;
	mi.ambient = m_ambient;
	mi.diffuse = m_diffuse;
	mi.specular = m_specular;
	mi.reflect = m_reflect;
	return mi;
}

void Material::SetAmbient(Vector4 ambient)
{
	m_ambient = ambient;
}

void Material::SetDiffuse(Vector4 diffuse)
{
	m_diffuse = diffuse;
}

void Material::SetSpecular(Vector4 specular)
{
	m_specular = specular;
}

void Material::SetReflect(Vector4 Reflect)
{
	m_reflect = Reflect;
}

void Material::Update()
{
}

void Material::Render()
{
}
