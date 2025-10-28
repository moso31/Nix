#pragma once
#include "BaseDefs/DX12.h"
#include "NXTerrainStreamingAsyncLoader.h"
#include "NXTexture.h"
#include "NXConstantBuffer.h"
#include "NXBuffer.h"
#include "NXInstance.h"

struct CBufferTerrainBatchConfig
{
	int processCount;		// 本次处理的纹理数量
	int _pad[3];
};

struct NXTerrainStreamNodeDescription
{
	Int2 pos;
	Int2 size;
	Int2 minmaxZ;
	Int2 lodbias;
	Int2 atlasUV;
};

/// <summary>
///	DX12管线，在主线程上运行。
/// 通过CS 将异步加载完成的多个小纹理合并到大Atlas
/// - input
///		- `m_batchTextures`到根参数
///		- `m_batchNodeDescriptions`到根参数
///		- `QuadTreeTexture`到根参数
///	- output
///		- `QuadTreeTexture`
///		- `NodeDescArray(GPU)`
///		- 最终的`Atlas`纹理
/// </summary>
class NXTerrainStreamingBatcher : public NXInstance<NXTerrainStreamingBatcher>
{
	// 每帧最多处理的"纹理包"数量
	// 目前每个"纹理包"包含：HeightMap、SplatMap 各一个
	static constexpr int MAX_PROCESSING_TEXTURE_PACKAGE = 8;

public:
	void Init();
    void PushCompletedTask(const NXTerrainStreamingLoadTextureResult& task);

	Ntr<NXTexture2D> GetSector2NodeIDTexture() const { return m_pSector2NodeIDTexture; }
	Ntr<NXBuffer> GetNodeDescriptionsArray() const { return m_pNodeDescriptionsArray; }
	Ntr<NXTexture2DArray> GetHeightMapAtlas() const { return m_pHeightMapAtlas; }
	Ntr<NXTexture2DArray> GetSplatMapAtlas() const { return m_pSplatMapAtlas; }

private:
	// in, 当前帧处理的纹理 
	std::vector<Ntr<NXTexture2D>> m_batchingHeightMap;
	std::vector<Ntr<NXTexture2D>> m_batchingSplatMap;

	// in-out, 纹理，记录sector-nodeID的映射
	Ntr<NXTexture2D> m_pSector2NodeIDTexture;

	// in-out, NodeDescriptionsArray
	Ntr<NXBuffer> m_pNodeDescriptionsArray;

	// out, 输出Atlas
	Ntr<NXTexture2DArray> m_pHeightMapAtlas;
	Ntr<NXTexture2DArray> m_pSplatMapAtlas;

	// 任务队列
	std::vector<NXTerrainStreamingLoadTextureResult> m_completedTasks; // 主线程的task
};
