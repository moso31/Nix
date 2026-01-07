#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"
#include "NXTerrainLODStreamConfigs.h"

struct StringDatas
{
	std::filesystem::path path; // 纹理文件路径
	std::string name; // 纹理资源名称，通常用于调试
};

struct TerrainStreamingLoadRequest
{
	// node所处世界坐标（左下角）
	Int2 positionWS;

	// 地形node的尺寸
	uint32_t size;

	// node的最大最小高度
	Vector2 minMaxZ;

	// 地形node在nodeDescArray中的索引
	uint32_t nodeDescArrayIndex;

	// 每个任务包负责加载的对应子纹理集
	StringDatas heightMap;
	StringDatas splatMap;
	StringDatas normalMap;

	// 是否替换nodeDescArray中的旧data
	// 旧data只需记录位置和size，能擦除就行
	// 注意如果是不需要替换, size=0
	Int2 replacePositionWS;
	int replaceSize;
};

class NXTexture2D;
struct NXTerrainStreamingLoadTextureResult
{
	// 除了把stringdatas换成实际纹理指针，其他和上面struct TerrainStreamingLoadRequest保持一致
	Int2 positionWS;
	uint32_t size;
	Vector2 minMaxZ;
	uint32_t nodeDescArrayIndex;
	Ntr<NXTexture2D> pHeightMap;
	Ntr<NXTexture2D> pSplatMap;
	Ntr<NXTexture2D> pNormalMap;
	Int2 replacePositionWS;
	int replaceSize;
};

class NXTerrainStreamingAsyncLoader
{
public:
	NXTerrainStreamingAsyncLoader() {};
	~NXTerrainStreamingAsyncLoader() {};

	void AddTask(const TerrainStreamingLoadRequest& task);
	void Update();

	int GetWorkingTaskNum() { return m_requestTasks.size() + m_loadingTasks.size(); }
	
	// 获取已完成的加载任务（主线程调用），注意会被直接消费掉（Get+Clear）
	std::vector<NXTerrainStreamingLoadTextureResult> ConsumeCompletedTasks();
	
private:
	std::vector<TerrainStreamingLoadRequest> m_requestTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_loadingTasks;
	std::vector<NXTerrainStreamingLoadTextureResult> m_computeTasks;
};
