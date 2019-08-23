#include "NXTransform.h"


NXTransform::NXTransform() :
	m_translation(0.0f),
	m_rotation(0.0f),
	m_scale(1.0f),
	m_worldMatrix(Matrix::Identity()),
	m_worldMatrixInv(Matrix::Identity())
{
}

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

Matrix NXTransform::GetWorldMatrix()
{
	return m_worldMatrix;
}

Matrix NXTransform::GetWorldMatrixInv()
{
	return m_worldMatrixInv;
}

void NXTransform::PrevUpdate()
{
	Matrix result = 
		Matrix::CreateTranslation(m_translation) * 
		Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z) * 
		Matrix::CreateScale(m_scale);
	m_worldMatrix = result;
	m_worldMatrixInv = result.Invert();
}
