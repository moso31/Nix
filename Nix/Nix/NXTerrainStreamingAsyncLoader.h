#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"

struct StringDatas
{
	std::filesystem::path path; // 纹理文件路径
	std::string name; // 纹理资源名称，通常用于调试
};

struct TerrainStreamingLoadRequest
{
	// node所处地形块ID
	Int2 terrainID;

	// 地形node相对当前地形左下角的位置，取整数
	Int2 relativePos; 

	// 高度范围
	Vector2 minMaxZ; 

	// 地形node的尺寸
	uint32_t size;

	// 地形node在nodeDescArray中的索引
	uint32_t nodeDescArrayIndex;

	// 每个任务包负责加载的对应子纹理集
	StringDatas heightMap;
	StringDatas splatMap;
};

class NXTexture2D;
struct NXTerrainStreamingLoadTextureResult
{
	// node所处地形块ID
	Int2 terrainID;

	// 地形node相对当前地形左下角的位置，取整数
	Int2 relativePos;

	// 高度范围
	Vector2 minMaxZ;

	// 地形node的尺寸
	uint32_t size;

	// 地形node在nodeDescArray中的索引
	uint32_t nodeDescArrayIndex;

	// 每个任务包异步加载的纹理
	Ntr<NXTexture2D> pHeightMap;
	Ntr<NXTexture2D> pSplatMap;
};

/// <summary>
/// 地形纹理 异步加载器
/// AddTask（主线程）：发请求
/// Update（异步线程）：接受请求，处理请求，并将完成的任务放到已完成队列
/// ConsumeCompletedTasks（主线程）：抽取已完成的任务
/// </summary>
class NXTerrainStreamingAsyncLoader
{
	static constexpr uint32_t s_maxRequestLimit = 8;
	static constexpr uint32_t s_maxComputeLimit = 8; // 每帧最多处理的完成任务数量
public:
	NXTerrainStreamingAsyncLoader() {};
	~NXTerrainStreamingAsyncLoader() {};

	void AddTask(const TerrainStreamingLoadRequest& task);
	void Update();
	
	// 获取已完成的加载任务（主线程调用），注意会被直接消费掉（Get+Clear）
	std::vector<NXTerrainStreamingLoadTextureResult> ConsumeCompletedTasks();
	
private:
	std::mutex m_tasksMutex;

	std::vector<TerrainStreamingLoadRequest> m_requestTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_loadingTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_computeTasks;
};
