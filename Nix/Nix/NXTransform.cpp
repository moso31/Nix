#include "NXTransform.h"


NXTransform::NXTransform() :
	m_translation(0.0f),
	m_eulerAngle(0.0f),
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

Vector3 NXTransform::GetRotation()
{
	return m_eulerAngle;
}

Quaternion NXTransform::GetQuaternion()
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

void NXTransform::SetQuaternion(const Quaternion &value)
{
	m_eulerAngle = value.EulerXYZ();
	m_rotation = value;
}

void NXTransform::SetRotation(const Vector3& value)
{
	m_eulerAngle = value;
	m_rotation = Quaternion::CreateFromYawPitchRoll(value.y, value.x, value.z);
}

void NXTransform::SetScale(const Vector3 &value)
{
	m_scale = value;
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
		Matrix::CreateFromXYZ(m_eulerAngle) *
		Matrix::CreateTranslation(m_translation);

	m_localMatrix = result;
	
	auto pParent = GetParent();
	while (pParent)
	{
		NXTransform* pTransform = dynamic_cast<NXTransform*>(pParent);
		if (pTransform)
		{
			pTransform->UpdateTransform();
			result *= pTransform->GetLocalMatrix();
		}
		pParent = pParent->GetParent();
	}
	
	m_worldMatrix = result;
	m_worldMatrixInv = result.Invert();
}
