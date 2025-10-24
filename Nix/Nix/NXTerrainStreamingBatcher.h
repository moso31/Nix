#pragma once
#include "BaseDefs/DX12.h"
#include "NXTerrainStreamingAsyncLoader.h"
#include <vector>

// 这个类只负责读取NXTerrainStreaming的完成task+输出
class NXTerrainStreamingBatcher
{
public:
    void Push(const NXTerrainStreamingLoadTextureResult& task);

	void Init();
	void Update();
	void Render();

private:
	std::vector<NXTerrainStreamingTask> m_completedTasks;
};
