#pragma once
struct NXGPUTerrainBlockData
{
	int x;
	int y;
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