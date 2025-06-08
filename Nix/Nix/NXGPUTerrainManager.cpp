#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

NXGPUTerrainManager::NXGPUTerrainManager() :
	m_terrainLodNum(6)
{
}

void NXGPUTerrainManager::Init()
{
	m_pTerrainLodBuffer.resize(m_terrainLodNum);
	m_gpuTerrainData.resize(m_terrainLodNum);

	return;

	uint32_t terrainNodeNum = m_pTerrains.size(); // 地形node数量
	for (int i = m_terrainLodNum - 1; i >= 0; i--)
	{
		auto& pBuffer = m_pTerrainLodBuffer[i];
		std::string name = "GPUTerrainLod" + std::to_string(i);

		pBuffer = new NXBuffer(name);
		pBuffer->Create(sizeof(NXGPUTerrainBlockData), terrainNodeNum);

		terrainNodeNum <<= 2; // 每个Lod级别的地形node数量是上一级的4倍
	}
}

void NXGPUTerrainManager::UpdateCameraParams(NXCamera* pCam)
{
	m_pTerrainParamsData.m_camPos = pCam->GetTranslation();
}

void NXGPUTerrainManager::UpdateLodParams(uint32_t lod)
{
	m_pTerrainParamsData.m_currLodLevel = lod;
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

Ntr<NXBuffer> NXGPUTerrainManager::GetTerrainLodBuffer(uint32_t lodLevel)
{
	if (lodLevel < m_pTerrainLodBuffer.size()) return m_pTerrainLodBuffer[lodLevel];
	return nullptr;
}
