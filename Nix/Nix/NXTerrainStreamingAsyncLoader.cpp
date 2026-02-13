#include "NXTerrainStreamingAsyncLoader.h"
#include "NXResourceManager.h"
#include "NXTexture.h"

void NXTerrainStreamingAsyncLoader::AddTask(const TerrainStreamingLoadRequest& task)
{
	m_requestTasks.push_back(task);
}

void NXTerrainStreamingAsyncLoader::Update()
{
	//printf("req: %d tasks\n", m_requestTasks.size());
	
	// 1. 请求队列->loading队列
	auto it = m_requestTasks.begin();
	while (it != m_requestTasks.end())
	{
		// 防止同时处理过多task
		if (m_loadingTasks.size() >= g_terrainStreamConfig.MaxRequestLimit) 
			break;

		NXTerrainStreamingLoadTextureResult nextTask;
		nextTask.positionWS = it->positionWS;
		nextTask.size = it->size;
		nextTask.nodeDescArrayIndex = it->nodeDescArrayIndex;
		nextTask.minMaxZ = it->minMaxZ;
		nextTask.pHeightMap = NXManager_Tex->CreateTexture2D(it->heightMap.name, it->heightMap.path, true);
		nextTask.pSplatMap  = NXManager_Tex->CreateTexture2D(it->splatMap.name, it->splatMap.path, true);
		nextTask.pNormalMap = NXManager_Tex->CreateTexture2D(it->normalMap.name, it->normalMap.path, true);
		nextTask.pAlbedoMap = NXManager_Tex->CreateTexture2D(it->albedoMap.name, it->albedoMap.path, true);
		nextTask.replacePositionWS = it->replacePositionWS;
		nextTask.replaceSize = it->replaceSize;

		m_loadingTasks.push_back(nextTask);
		it = m_requestTasks.erase(it);
	} 
	
	// 2. loading->Completed队列
	int loadingCnt = 0;
	for (auto it = m_loadingTasks.begin(); it != m_loadingTasks.end(); loadingCnt++)
	{
		auto& task = *it;

		if (m_computeTasks.size() >= g_terrainStreamConfig.MaxComputeLimit)
			break;

		if (it->pHeightMap.IsValid() && it->pHeightMap->IsLoadReady() && 
			it->pSplatMap.IsValid() && it->pSplatMap->IsLoadReady() && 
			it->pNormalMap.IsValid() && it->pNormalMap->IsLoadReady() &&
			it->pAlbedoMap.IsValid() && it->pAlbedoMap->IsLoadReady())
		{
			m_computeTasks.push_back(std::move(*it));
			it = m_loadingTasks.erase(it);
		}
		else
		{
			++it;
		}
	}
}

std::vector<NXTerrainStreamingLoadTextureResult> NXTerrainStreamingAsyncLoader::ConsumeCompletedTasks()
{
	// 一次性把所有的已完成task取走
	std::vector<NXTerrainStreamingLoadTextureResult> result = std::move(m_computeTasks);
	m_computeTasks.clear();
	return result;
}
