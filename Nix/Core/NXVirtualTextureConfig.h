#pragma once

struct NXVirtualTextureConfig
{
	// VT���ͻ���һ֡���Ӵ��̼��ص�����Package����
	// һ��Package = һ�飬�����߶�ͼ��Splatͼ��δ������������
	int MaxLoadTilesFromDiskAtOnce = 4;

	// VT����һ֡��ദ�������Package����
	int MaxProcessingBatchesAtOnce = 4;

	// ÿ֡����ĵ�������View�������ޣ�SplatMap��HeightMap��
	int MaxRequestTerrainViewsPerUpdate = 16;

	// ÿ֡�������������View��������
	int MaxRequestDecalViewsPerUpdate = 16;

	// ����ҳ����Tile�����ش�С
	int PhysicalPageTileSize = 256; 
};

inline NXVirtualTextureConfig g_VTConfig;