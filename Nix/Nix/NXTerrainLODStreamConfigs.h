#pragma once
#include <cfloat>
#include <cstdint>

struct NXTerrainLODStreamConfigsStruct
{
	// ========================================
	// NXTerrainLODStreamData 配置
	// ========================================

	// HeightMap Atlas 的单个纹理尺寸（65 x 65）
	static constexpr int AtlasHeightMapSize = 65;

	// SplatMap Atlas 的单个纹理尺寸（65 x 65）
	static constexpr int AtlasSplatMapSize = 65;

	// Atlas Texture2DArray 的层数（最大可加载的节点数量）
	static constexpr int AtlasLayerCount = 1024;

	// Sector2NodeID 纹理的尺寸
	static constexpr int Sector2NodeIDTexSize = 256;

	// ========================================
	// NXTerrainLODStreamer 配置
	// ========================================

	// 各LOD级别对应的距离范围
	static constexpr float DistRanges[6] = { 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, FLT_MAX };

	// 几档LOD
	static constexpr int LODSize = 6;

	// 最大节点层级（0~5 共6层）
	static constexpr int MaxNodeLevel = LODSize - 1;

	// 预分配已加载节点描述数组的初始大小
	static constexpr int NodeDescArrayInitialSize = 1024;

	// ========================================
	// NXTerrainStreamingAsyncLoader 配置
	// ========================================

	// 每帧最多请求几组任务。注意每个任务对应一个地形节点Task=加载4N张纹理。
	static constexpr uint32_t MaxRequestLimit = 4;

	// 每帧最多处理的完成任务数量
	static constexpr uint32_t MaxComputeLimit = 4;
};

inline NXTerrainLODStreamConfigsStruct g_terrainStreamConfig;
