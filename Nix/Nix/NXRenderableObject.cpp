#include "NXRenderableObject.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXRenderableObject::NXRenderableObject() :
	m_geoTranslation(Vector3(0.0f)),
	m_geoRotation(Vector3(0.0f)),
	m_geoScale(Vector3(1.0f)),
	m_rotMatrix(Matrix::Identity()),
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

	m_worldMatrix = geoMatrix * result;
	m_worldMatrixInv = result.Invert();
}
