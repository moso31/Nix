#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

NXGPUTerrainManager::NXGPUTerrainManager()
{
}

void NXGPUTerrainManager::Init()
{
	m_terrainLODDistProfile = { 12600, 6200, 3000, 1400, 600, 200 }; // 6级LOD
	m_pTerrainLodBuffer.resize(m_terrainLODDistProfile.size());
	m_gpuTerrainData.resize(m_terrainLODDistProfile.size());

	std::vector<uint32_t> maxLODBlockNum;
	for (int i = 0; i < m_terrainLODDistProfile.size(); i++)
	{
		float M = (float)(2048 >> i); // 单个lod格的大小
		float r = (float)m_terrainLODDistProfile[i]; // 对应lod等级的加载半径

		// 计算每个LOD等级的极限加载块数
		float val = (r / M + 1.0f);
		uint32_t blockNum = (uint32_t)(XM_PI * val * val);
		maxLODBlockNum.push_back(blockNum);
	}

	for (int i = 0; i < m_terrainLODDistProfile.size(); i++)
	{
		auto& pBuffer = m_pTerrainLodBuffer[i];
		std::string name = "GPUTerrainLod" + std::to_string(i);

		pBuffer = new NXBuffer(name);
		pBuffer->Create(sizeof(float) * 8, maxLODBlockNum[i]);
	}
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

void NXGPUTerrainManager::UpdateGPUTerrainNodes(NXCamera* pCamera)
{
	bool isFirstBlock = true;
	for (auto& pTerrain : m_pTerrains)
	{
		pTerrain->GetGPUTerrainNodes(pCamera->GetTranslation(), m_terrainLODDistProfile, m_gpuTerrainData, isFirstBlock); // 第一个block清除之前的旧数据，后续的不用
		isFirstBlock = false;
	}

	for (int i = 0; i < m_pTerrainLodBuffer.size(); i++)
	{
		m_pTerrainLodBuffer[i]->Set(m_gpuTerrainData[i].data(), m_gpuTerrainData[i].size());
	}
}

Ntr<NXBuffer> NXGPUTerrainManager::GetTerrainLodBuffer(uint32_t lodLevel)
{
	if (lodLevel < m_pTerrainLodBuffer.size()) return m_pTerrainLodBuffer[lodLevel];
	return nullptr;
}
