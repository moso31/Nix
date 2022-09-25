#include "NXRenderableObject.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXIntersection.h"
#include "NXPrefab.h"
#include "NXPrimitive.h"

NXRenderableObject::NXRenderableObject() :
	m_geoTranslation(Vector3(0.0f)),
	m_geoRotation(Vector3(0.0f)),
	m_geoScale(Vector3(1.0f)),
	m_transformWorldMatrix(Matrix::Identity()),
	m_transformWorldMatrixInv(Matrix::Identity()),
	m_bIsVisible(true),
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
	AABB::Transform(m_localAABB, m_worldMatrix, worldAABB);
	return worldAABB;
}

AABB NXRenderableObject::GetAABBLocal()
{
	return m_localAABB;
}

bool NXRenderableObject::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	bool bSuccess = false;

	// ray-aabb
	float outAABBDist;
	if (worldRay.IntersectsFast(GetAABBWorld(), outAABBDist))
	{
		bool isInAABB = outAABBDist < 0;
		bool isNearestMaybe = (!isInAABB && outAABBDist < outDist) || isInAABB;

		if (isNearestMaybe)
		{
			float dist = outDist;
			NXHit hitInfo;

			auto pPrim = this->IsPrimitive();
			if (pPrim)
			{
				if (pPrim->RayCastPrimitive(worldRay, hitInfo, dist) && dist < outDist)
				{
					outHitInfo = hitInfo;
					outDist = dist;
					bSuccess = true;
				}
			}

			for (auto pChild : GetChilds())
			{
				auto pChildRenderObj = pChild->IsRenderableObject();
				if (pChildRenderObj)
				{
					if (pChildRenderObj->RayCast(worldRay, hitInfo, dist) && dist < outDist)
					{
						outHitInfo = hitInfo;
						outDist = dist;
						bSuccess = true;
					}
				}
			}
		}
	}

	return bSuccess;
}

void NXRenderableObject::InitAABB()
{
	AABB worldAABB = GetAABBWorld();
	for (auto pChild : GetChilds())
	{
		auto pChildRenderableObj = pChild->IsRenderableObject();
		if (pChildRenderableObj)
		{
			pChildRenderableObj->InitAABB();
			AABB childWorldAABB = pChildRenderableObj->GetAABBWorld();

			AABB::CreateMerged(worldAABB, worldAABB, childWorldAABB);
		}
	}

	AABB::Transform(worldAABB, m_worldMatrixInv, m_localAABB);
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
	if (pParent && pParent->IsTransform())
	{
		if (pParent->IsRenderableObject())
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
