#include "NXTransform.h"

NXTransform::NXTransform() :
	m_translation(0.0f),
	m_eulerAngle(0.0f),
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

Vector3 NXTransform::GetScale()
{
	return m_scale;
}

void NXTransform::SetTranslation(const Vector3 &value)
{
	m_translation = value;
	printf("name: %s        Translation: %f %f %f\n", m_name.c_str(), m_translation.x, m_translation.y, m_translation.z);
}

void NXTransform::SetRotation(const Vector3& value)
{
	m_eulerAngle = value;
	printf("name: %s        Rotation:    %f %f %f\n", m_name.c_str(), m_eulerAngle.x, m_eulerAngle.y, m_eulerAngle.z);
}

void NXTransform::SetScale(const Vector3 &value)
{
	m_scale = value;
	printf("name: %s        Scaling:     %f %f %f\n", m_name.c_str(), m_scale.x, m_scale.y, m_scale.z);
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
		Matrix::CreateFromRollPitchYaw(m_eulerAngle) *
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
