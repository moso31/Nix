#include "NXTransform.h"

NXTransform::NXTransform() :
	m_translation(0.0f),
	m_eulerAngle(0.0f),
	m_scale(1.0f),
	m_rotation(Quaternion()),
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
	return m_eulerAngle;
}

Vector3 NXTransform::GetScale()
{
	return m_scale;
}

void NXTransform::SetTranslation(const Vector3 &value)
{
	m_translation = value;
	UpdateTransform();
}

void NXTransform::SetRotation(const Vector3& value)
{
	m_eulerAngle = value;
	Vector3 k = value * 180.0f / XM_PI;
	UpdateTransform();
}

void NXTransform::SetRotation(const Quaternion& value)
{
	m_rotation = value;
	UpdateTransform();
}

void NXTransform::SetScale(const Vector3 &value)
{
	m_scale = value;
	UpdateTransform();
}

Matrix NXTransform::GetLocalMatrix()
{
	return m_localMatrix;
}

Matrix NXTransform::GetWorldMatrix()
{
	return m_worldMatrix;
}

Matrix NXTransform::GetWorldMatrixInv()
{
	return m_worldMatrixInv;
}

void NXTransform::UpdateTransform()
{
	Matrix result =
		Matrix::CreateScale(m_scale) *
		Matrix::CreateFromZXY(m_eulerAngle) *
		Matrix::CreateTranslation(m_translation);

	m_localMatrix = result;

	NXTransform* pTransform = dynamic_cast<NXTransform*>(GetParent());
	if (pTransform)
		result *= pTransform->GetWorldMatrix();
	
	m_worldMatrix = result;
	m_worldMatrixInv = result.Invert();
}
