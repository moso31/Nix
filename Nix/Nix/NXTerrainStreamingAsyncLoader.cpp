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
	for (const auto& task : m_requestTasks)
	{
		NXTerrainStreamingLoadTextureResult nextTask;
		nextTask.terrainID = task.terrainID;
		nextTask.relativePos = task.relativePos;
		nextTask.size = task.size;
		nextTask.nodeDescArrayIndex = task.nodeDescArrayIndex;
		nextTask.pHeightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(task.heightMap.name, task.heightMap.path);
		nextTask.pSplatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(task.splatMap.name, task.splatMap.path);

		m_loadingTasks.push_back(nextTask);
	}
	m_requestTasks.clear(); 
	
	// 2. loading->Completed队列
	int loadingCnt = 0;
	for (auto it = m_loadingTasks.begin(); it != m_loadingTasks.end(); loadingCnt++)
	{
		// 防止同时处理过多task
		if (loadingCnt >= s_maxRequestLimit) 
			break;

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
