#include "NXTerrainStreamingAsyncLoader.h"
#include "NXResourceManager.h"
#include "NXTexture.h"

void NXTerrainStreamingAsyncLoader::AddTask(const TerrainStreamingLoadRequest& task)
{
	std::lock_guard<std::mutex> lock(m_tasksMutex);
	m_requestTasks.push_back(task);
}

void NXTerrainStreamingAsyncLoader::Update()
{
	std::lock_guard<std::mutex> lock(m_tasksMutex);
	
	// 1. 请求队列->loading队列
	auto it = m_requestTasks.begin();
	while (it != m_requestTasks.end())
	{
		// 防止同时处理过多task
		if (m_loadingTasks.size() >= s_maxRequestLimit) 
			break;

		NXTerrainStreamingLoadTextureResult nextTask;
		nextTask.positionWS = it->positionWS;
		nextTask.size = it->size;
		nextTask.nodeDescArrayIndex = it->nodeDescArrayIndex;
		nextTask.minMaxZ = it->minMaxZ;
		nextTask.pHeightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(it->heightMap.name, it->heightMap.path);
		nextTask.pSplatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(it->splatMap.name, it->splatMap.path);
		nextTask.replacePositionWS = it->replacePositionWS;
		nextTask.replaceSize = it->replaceSize;

		m_loadingTasks.push_back(nextTask);
		it = m_requestTasks.erase(it);
	} 
	
	// 2. loading->Completed队列
	int loadingCnt = 0;
	for (auto it = m_loadingTasks.begin(); it != m_loadingTasks.end(); loadingCnt++)
	{
		if (m_computeTasks.size() >= s_maxComputeLimit)
			break;

		if (it->pHeightMap.IsValid() && it->pHeightMap->IsLoadReady() && it->pSplatMap.IsValid() && it->pSplatMap->IsLoadReady())
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
	// 一次性把所有的已完成task取走，放到主线程那边去处理
	std::lock_guard<std::mutex> lock(m_tasksMutex);
	std::vector<NXTerrainStreamingLoadTextureResult> result = std::move(m_computeTasks);
	m_computeTasks.clear();
	return result;
}
