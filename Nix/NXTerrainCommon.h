#pragma once
#include "BaseDefs/Math.h"

struct NXGPUTerrainBlockData
{
	int x;
	int y;
};

struct MinMaxZMap
{
	float minVal;
	float maxVal;
};

struct NXTerrainNodeId
{
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

struct NXGPUTerrainParams
{
	Vector3 m_camPos;
	float m_nodeWorldScale; // lod等级的单个node的世界大小
	uint32_t m_currLodLevel;
	float m_currLodDist;
};

struct NXGPUTerrainPatcherParams
{
	Matrix m_mxWorld;
	Vector3 m_pos;
	int m_sliceIndex;
	Vector2 m_uv;
	Vector2 m_terrainOrigin;
};

struct NXGPUTerrainSupport
{
	int m_blockMinIdX;
	int m_blockMinIdY;
	int m_terrainBlockWidth;
	float m_debugParam = 0.0f;
};
