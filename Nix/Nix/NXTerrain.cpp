#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "NXTimer.h"
#include "NXAllocatorManager.h"
#include "NXGPUTerrainManager.h"

NXTerrain::NXTerrain(int gridSize, int worldSize, NXTerrainNodeId terrainNodeId, int terrainId) :
	m_gridSize(gridSize),
	m_worldSize(worldSize),
	m_terrainNodeId(terrainNodeId),
	m_terrainId(terrainId)
{
}

void NXTerrain::SetTerrainLayer(NXTerrainLayer* pTerrainLayer)
{
	m_pTerrainLayer = pTerrainLayer;
}

void NXTerrain::AddSubMesh(NXSubMeshBase* pSubMesh)
{
	m_pSubMesh.reset(pSubMesh);
}


bool NXTerrain::RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	NXHit hit;
	hit.pSubMesh = m_pSubMesh.get();
	outDist = 0.0f;
	outHitInfo = hit;
	return true;
}

void NXTerrain::InitAABB()
{
	Vector3 vMin(0.0f, 0.0f, 0.0f);
	Vector3 vMax((float)m_worldSize, 1000.0f, (float)m_worldSize);
	m_localAABB = AABB(vMin, vMax);

	NXRenderableObject::InitAABB();
}

void NXTerrain::Update(ID3D12GraphicsCommandList* pCmdList)
{
}
