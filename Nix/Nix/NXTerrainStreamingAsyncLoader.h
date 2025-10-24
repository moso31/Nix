#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"

struct NXTerrainStreamingLoadTask_EachTexture
{
	std::filesystem::path path; // 纹理文件路径
	std::string name; // 纹理资源名称，通常用于调试
};

struct TerrainStreamingLoadRequest
{
	// 要加载的地形块ID
	Int2 terrainID; 

	// 要加载的地形node的相对编号
	Int2 relativePosID; 

	// 要加载的地形node的尺寸
	uint32_t size; 

	// 每个任务包负责加载的对应子纹理集
	NXTerrainStreamingLoadTask_EachTexture heightMap;
	NXTerrainStreamingLoadTask_EachTexture splatMap;
};

class NXTexture2D;
struct NXTerrainStreamingLoadTextureResult
{
	Int2 terrainID;
	Int2 relativePosID;
	uint32_t size;

	Ntr<NXTexture2D> pHeightMap;
	Ntr<NXTexture2D> pSplatMap;
};

class NXTerrainStreamingAsyncLoader
{
	static constexpr uint32_t s_maxRequestLimit = 8;
public:
	NXTerrainStreamingAsyncLoader() {};
	~NXTerrainStreamingAsyncLoader() {};

	void AddTask(const TerrainStreamingLoadRequest& task);
	void Update();
	
	// 获取已完成的加载任务（主线程调用），注意会被直接消费掉（Get+Clear）
	std::vector<NXTerrainStreamingLoadTextureResult> ConsumeCompletedTasks();
	
private:
	std::vector<TerrainStreamingLoadRequest> m_requestTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_loadingTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_completedTasks;
	std::mutex m_tasksMutex; // 保护任务队列的线程安全
};
