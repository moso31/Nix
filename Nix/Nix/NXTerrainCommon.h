#pragma once
#include "BaseDefs/Math.h"

struct MinMaxZMap
{
	float minVal;
	float maxVal;
};

struct NXTerrainNodeId
{
	NXTerrainNodeId() : x(SHRT_MIN), y(SHRT_MIN) {}
	NXTerrainNodeId(short x, short y) : x(x), y(y) {}

	bool operator ==(NXTerrainNodeId other) const
	{
		return x == other.x && y == other.y;
	}

	short x;
	short y;
};

template<>
struct std::hash<NXTerrainNodeId> 
{
	size_t operator()(const NXTerrainNodeId& id) const
	{
		return (static_cast<std::size_t>(id.x) << 16) ^ static_cast<std::size_t>(id.y);
	}
};

struct NXTerrainConfig
{
	// 地形块大小
	int TerrainSize = 2048;

	// sector大小
	int SectorSize = 64;

	// 最小地形坐标
	Int2 MinTerrainPos = Int2(-8192, -8192); 

	// 最小SectorID
	Int2 MinSectorID = MinTerrainPos / SectorSize;
};

inline NXTerrainConfig g_terrainConfig;
