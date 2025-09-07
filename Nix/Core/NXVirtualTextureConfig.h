#pragma once

struct NXVirtualTextureConfig
{
	// VT流送机制一帧最多从磁盘加载的纹理Package数量
	// 一个Package = 一组，包括高度图、Splat图、未来还包括贴花
	int MaxLoadTilesFromDiskAtOnce = 4;

	// VT管线一帧最多处理的纹理Package数量
	int MaxProcessingBatchesAtOnce = 4;

	// 每帧请求的地形纹理View数量上限（SplatMap、HeightMap）
	int MaxRequestTerrainViewsPerUpdate = 16;

	// 每帧请求的贴花纹理View数量上限
	int MaxRequestDecalViewsPerUpdate = 16;

	// 物理页单个Tile的像素大小
	int PhysicalPageTileSize = 256; 
};

inline NXVirtualTextureConfig g_VTConfig;