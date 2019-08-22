#include "NXTransform.h"


Vector3 NXTransform::GetTranslation()
{
	return m_translation;
}

Vector3 NXTransform::GetRotation()
{
	return m_rotation;
}

Vector3 NXTransform::GetScale()
{
	return m_scale;
}

void NXTransform::SetTranslation(Vector3 value)
{
	m_translation = value;
}

void NXTransform::SetRotation(Vector3 value)
{
	m_rotation = value;
}

void NXTransform::SetScale(Vector3 value)
{
	m_scale = value;
}
