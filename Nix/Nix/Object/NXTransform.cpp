#include "NXTransform.h"
#include "Object\NXTransform.h"


NXTransform::NXTransform() :
	m_translation(0.0f),
	m_rotation(Quaternion()),
	m_scale(1.0f),
	m_worldMatrix(Matrix::Identity()),
	m_worldMatrixInv(Matrix::Identity())
{
}

Vector3 NXTransform::GetTranslation()
{
	return m_translation;
}

Quaternion NXTransform::GetRotation()
{
	return m_rotation;
}

Vector3 NXTransform::GetScale()
{
	return m_scale;
}

void NXTransform::SetTranslation(const Vector3 &value)
{
	m_translation = value;
}

void NXTransform::SetRotation(const Quaternion &value)
{
	m_rotation = value;
}

void NXTransform::SetRotation(const Vector3& value)
{
	m_rotation = Quaternion::CreateFromYawPitchRoll(value.y, value.x, value.z);
}

void NXTransform::SetScale(const Vector3 &value)
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
		Matrix::CreateFromQuaternion(m_rotation) *
		//Matrix::CreateFromXYZ(m_rotation) *
		Matrix::CreateScale(m_scale);
	m_worldMatrix = result;
	m_worldMatrixInv = result.Invert();
}
