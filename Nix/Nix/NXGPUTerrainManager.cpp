#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

NXGPUTerrainManager::NXGPUTerrainManager()
{
}

void NXGPUTerrainManager::Init()
{
	m_terrainLODDistProfile = { 12600, 6200, 3000, 1400, 600, 200 }; // 6��LOD
}

void NXGPUTerrainManager::AddSceneTerrains(NXScene* pScene)
{
	for (auto& pTerrain : pScene->GetTerrains())
	{
		m_pTerrains.push_back(pTerrain);
	}
}

void NXGPUTerrainManager::AddTerrain(Ntr<NXTerrain> pTerrain)
{
	m_pTerrains.push_back(pTerrain);
}

void NXGPUTerrainManager::GetGPUTerrainNodes(NXCamera* pCamera)
{
	bool isFirstBlock = true;
	for (auto& pTerrain : m_pTerrains)
	{
		pTerrain->GetGPUTerrainNodes(pCamera->GetTranslation(), m_terrainLODDistProfile, m_gpuTerrainData, isFirstBlock); // ��һ��block���֮ǰ�ľ����ݣ������Ĳ���
		isFirstBlock = false;
	}
}
