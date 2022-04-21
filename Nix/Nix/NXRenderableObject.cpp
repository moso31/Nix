#include "NXRenderableObject.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"

NXRenderableObject::NXRenderableObject()
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
