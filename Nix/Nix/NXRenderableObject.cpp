#include "NXRenderableObject.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXRenderableObject::NXRenderableObject() :
	m_geoTranslation(Vector3(0.0f)),
	m_geoRotation(Vector3(0.0f)),
	m_geoScale(Vector3(1.0f)),
	m_transformWorldMatrix(Matrix::Identity()),
	m_transformWorldMatrixInv(Matrix::Identity()),
	NXTransform()
{
	m_type = NXType::ePrefab;
}

void NXRenderableObject::Release()
{
	NXObject::Release();
}

AABB NXRenderableObject::GetAABBWorld()
{
	AABB worldAABB;
	AABB::Transform(m_aabb, m_worldMatrix, worldAABB);
	return worldAABB;
}

AABB NXRenderableObject::GetAABBLocal()
{
	return m_aabb;
}

void NXRenderableObject::UpdateTransform()
{
	Matrix geoMatrix = Matrix::CreateScale(m_geoScale) *
		Matrix::CreateFromZXY(m_geoRotation) *
		Matrix::CreateTranslation(m_geoTranslation);

	Matrix result = Matrix::CreateScale(m_scale) *
		Matrix::CreateFromZXY(m_eulerAngle) *
		Matrix::CreateTranslation(m_translation);

	m_localMatrix = result;

	m_transformWorldMatrix = result;

	auto pParent = GetParent();
	if (pParent->IsTransformType())
	{
		if (pParent->GetType() == NXType::ePrimitive || pParent->GetType() == NXType::ePrefab)
		{
			NXRenderableObject* pRenObj = static_cast<NXRenderableObject*>(pParent);
			m_transformWorldMatrix *= pRenObj->m_transformWorldMatrix;
		}
		else
		{
			NXTransform* pTransform = static_cast<NXTransform*>(pParent);
			m_transformWorldMatrix *= pTransform->GetWorldMatrix();
		}
	}

	m_transformWorldMatrixInv = m_transformWorldMatrix.Invert();

	m_worldMatrix = geoMatrix * m_transformWorldMatrix;
	m_worldMatrixInv = m_worldMatrix.Invert();
}
