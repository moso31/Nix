#include "NXTerrainStreamingAsyncLoader.h"
#include "NXTextureResourceManager.h"

void NXTerrainStreamingAsyncLoader::AddTask(const TerrainStreamingLoadRequest& task)
{
	std::lock_guard<std::mutex> lock(m_tasksMutex);
	m_requestTasks.push_back(task);
}

void NXTerrainStreamingAsyncLoader::Update()
{
	// 快照 避免竞争
	std::vector<TerrainStreamingLoadRequest> reqTasks;
	
	{
		std::lock_guard<std::mutex> lock(m_tasksMutex);
		if (m_requestTasks.empty())
			return;
			
		reqTasks = std::move(m_requestTasks);
		m_requestTasks.clear();
	}
	
	for (const auto& task : reqTasks)
	{
		NXTerrainStreamingLoadTextureResult nextTask;
		nextTask.terrainID = task.terrainID;
		nextTask.relativePosID = task.relativePosID;
		nextTask.size = task.size;

		nextTask.heightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strHeightMapName, texHeightMapPath, task.tileRelativePos, task.tileSize + 1);
		nextTask.splatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strSplatMapName, texSplatMapPath, task.tileRelativePos, task.tileSize + 1);

		{
			std::lock_guard<std::mutex> lock(m_tasksMutex);
			m_completedTasks.push_back(nextTask);
		}
	}
}

std::vector<NXTerrainStreamingLoadTextureResult> NXTerrainStreamingAsyncLoader::GetCompletedTasks()
{
	// 一次性把所有的已完成task取走，放到主线程那边去处理
	std::lock_guard<std::mutex> lock(m_tasksMutex);
	std::vector<NXTerrainStreamingLoadTextureResult> result = std::move(m_completedTasks);
	m_completedTasks.clear();
	return result;
}
