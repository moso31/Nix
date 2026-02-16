#ifndef TERRAIN_COMMON_FX
#define TERRAIN_COMMON_FX

// 通用宏定义
#define NodeDescArrayNum 1024
#define NodeDescUpdateIndicesNum 16

// 最小地形坐标
#define MinTerrainCoord -8192

// 地形节点描述结构体
struct CBufferTerrainNodeDescription
{
	// 地形左下角XZ节点坐标（左手坐标系）
    int2 positionWS;

	// 节点的minmaxZ数据
    float2 minmaxZ;

	// 节点大小，一定是2的整数幂
    uint size;

    uint3 padding; // 16 byte align
};

// 地形节点更新信息结构体
struct CBufferTerrainNodeDescUpdateInfo
{
	// 要替换的索引
    int newIndex;

	// 被替换的旧信息和大小
	// 注意如果是不需要replace，则size = 0;
    int2 replacePosWS;
    int replaceSize;
};

struct TerrainPatchData
{
    int2 patchOrigin;
    int patchSize;
    int atlasIndex;
    int2 nodeOrigin; // nodeSize = f(patchSize)
    float2 patchOriginPixelPos;
};

struct TerrainPatchDrawIndexArgs
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation; // 0
    int baseVertexLocation; // 0
    uint startInstanceLocation; // 0
};

#endif // TERRAIN_COMMON_FX
