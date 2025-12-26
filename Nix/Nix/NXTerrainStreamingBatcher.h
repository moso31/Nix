//#pragma once
//#include "BaseDefs/DX12.h"
//#include "NXInstance.h"
//#include "NXTerrainStreamingAsyncLoader.h"
//#include "NXTexture.h"
//#include "NXConstantBuffer.h"
//#include "NXBuffer.h"
//#include <vector>
//
//struct CBufferTerrainBatchConfig
//{
//	int processCount;		// 本次处理的纹理数量
//	int _pad[3];
//};
//
//struct NXTerrainStreamBatcherNodeDescription
//{
//	Int2 relativePos;
//	uint32_t size;
//	uint32_t nodeDescArrayIndex;
//};
//
//struct NXTerrainStreamNodeDescriptionGPU
//{
//	Int2 relativePos;	// 相对地形左下角的位置
//	uint32_t size;		// 节点尺寸
//	Vector2 minMaxZ;	// 高度范围
//	Vector2 atlasUV;	// 在Atlas中的UV坐标（左下角）
//};
//
///// <summary>
/////	DX12管线，在主线程上运行。
///// 通过CS 将异步加载完成的多个小纹理合并到大Atlas
///// - input
/////		- 多个 HeightMap 纹理
///// 	- 多个 SplatMap 纹理
/////		- 
/////	- output
/////		- `QuadTreeTexture`
/////		- `NodeDescArray(GPU)`
/////		- 最终的`Atlas`纹理
///// </summary>
//class NXTerrainStreamingBatcher : public NXInstance<NXTerrainStreamingBatcher>
//{
//	// 每帧最多处理的"纹理包"数量
//	// 目前每个"纹理包"包含：HeightMap、SplatMap 各一个
//	static constexpr int MAX_PROCESSING_TEXTURE_PACKAGE = 8;
//
//public:
//	void Init();
//    void PushCompletedTask(const NXTerrainStreamingLoadTextureResult& task);
//
//	Ntr<NXTexture2D> GetSector2NodeIDTexture() const { return m_pSector2NodeIDTexture; }
//	Ntr<NXTexture2DArray> GetHeightMapAtlas() const { return m_pHeightMapAtlas; }
//	Ntr<NXTexture2DArray> GetSplatMapAtlas() const { return m_pSplatMapAtlas; }
//	Ntr<NXBuffer> GetNodeDescriptionsArrayGPU() const { return m_pNodeDescriptionsArray; }
//
//	NXConstantBuffer<std::vector<NXTerrainStreamBatcherNodeDescription>>& GetBatchingNodeDescCBuffer() { return m_batchingNodeDesc; }
//
//private:
//	// in, 当前帧处理的纹理 
//	std::vector<Ntr<NXTexture2D>> m_batchingHeightMap;
//	std::vector<Ntr<NXTexture2D>> m_batchingSplatMap;
//
//	// in, CBuffer，记录要写到Atlas的纹理信息
//	std::vector<NXTerrainStreamBatcherNodeDescription> m_batchingNodeDescData;
//	NXConstantBuffer<std::vector<NXTerrainStreamBatcherNodeDescription>> m_batchingNodeDesc;
//
//	// out, 纹理，记录sector-nodeID的映射
//	Ntr<NXTexture2D> m_pSector2NodeIDTexture;
//
//	// out, NodeDescriptionsArray
//	Ntr<NXBuffer> m_pNodeDescriptionsArray;
//
//	// out, 输出Atlas
//	Ntr<NXTexture2DArray> m_pHeightMapAtlas;
//	Ntr<NXTexture2DArray> m_pSplatMapAtlas;
//
//	// 任务队列
//	std::vector<NXTerrainStreamingLoadTextureResult> m_completedTasks; // 主线程的task
//};
