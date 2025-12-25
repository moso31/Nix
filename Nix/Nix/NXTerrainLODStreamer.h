#pragma once
#include "NXTerrainLODStreamData.h"

class NXTerrainStreamingAsyncLoader;
class NXTerrainStreamingBatcher;
struct NXTerrainLODQuadTreeNode
{
	Int2 terrainID; // 地形ID
	Int2 positionWS; // 地形左下角XZ节点坐标（左手坐标系）
	uint32_t size; // 节点大小，一定是2的整数幂

	NXTerrainLODQuadTreeNode GetChildNode(int index) const
	{
		NXTerrainLODQuadTreeNode childNode;

		childNode.terrainID = terrainID;
		childNode.size = size >> 1;
		switch (index)
		{
		case 0: // 左下子块
			childNode.positionWS = positionWS;
			break;
		case 1: // 右下子块
			childNode.positionWS = positionWS + Int2(size >> 1, 0);
			break;	
		case 2: // 左上子块
			childNode.positionWS = positionWS + Int2(0, size >> 1);
			break;
		case 3: // 右上子块
			childNode.positionWS = positionWS + Int2(size >> 1, size >> 1);
			break;
		}

		return childNode;
	}

	// 获取节点所在的LOD等级
	// 2048 -> level 0; 1024 -> level 1; 512 -> level 2 ...
	uint32_t GetLevel() const 
	{
		uint32_t level = 0;
		uint32_t tempSize = size;
		while (tempSize > 1)
		{
			tempSize >>= 1;
			level++;
		}
		return 11 - level; // 2048 = 2^11 = lod0.
	}
};

struct NXTerrainLODQuadTreeNodeDescription
{
	NXTerrainLODQuadTreeNode data;

	// 是否已经异步加载完成
	bool isValid = false;

	// 是否正在异步加载中
	bool isLoading = false;

	// 记录上次更新的帧数；
	// 如果某个node长时间没有被访问到，则可以考虑卸载
	uint64_t lastUpdatedFrame = 0;
};

class NXCamera;
class NXScene;
/// <summary>
/// 四叉树地形流式加载的核心
/// 负责基于当前场景的相机距离，决定需要流式加载哪些地形节点的纹理
/// 使用位置固定的LRU Cache（对应类中m_nodeDescArrayInternal）来管理已经加载的节点，如果出现Cache未记录节点，异步加载
/// </summary>
class NXTerrainLODStreamer
{
public:
	static constexpr int s_maxNodeLevel = 5; // 最大节点层级 0~5 共6层
	static constexpr float s_distRanges[6] = { 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, FLT_MAX }; // 这样写可以不在cpp再初始化一次 很方便
	static constexpr int s_nodeDescArrayInitialSize = 1024; // 预分配已加载节点描述数组的初始大小

public:
	NXTerrainLODStreamer();
	~NXTerrainLODStreamer();

	void Init(NXScene* m_pScene);

	// 每帧更新
	void Update();
	void UpdateAsyncLoader();

	void ProcessCompletedStreamingTask();

	NXTerrainLODStreamData& GetStreamingData() { return m_streamData; }

private:
	// 获取6档距离内的节点，输出一个list[6]；只要是当前档次距离能覆盖的，统统加入到预加载队列
	void GetNodeDatasInternal(std::vector<std::vector<NXTerrainLODQuadTreeNode>>& oNodeDataList, const NXTerrainLODQuadTreeNode& node);

	// 加载 minmaxZ 数据（用于地形剔除等）
	void LoadMinmaxZData();

private:
	std::filesystem::path m_terrainWorkingDir = "D:\\NixAssets\\Terrain";

	// 每个地形是一个四叉树
	std::vector<NXTerrainLODQuadTreeNode> m_terrainRoots;

	// "已经加载"到Atlas的节点
	// 长度固定，初始化直接resize
	std::vector<NXTerrainLODQuadTreeNodeDescription> m_nodeDescArrayInternal;

	// 异步加载器，异步读取tile纹理
	NXTerrainStreamingAsyncLoader* m_asyncLoader;

	// 场景指针
	NXScene* m_pScene;

	// 流式加载所使用的各种数据
	NXTerrainLODStreamData m_streamData;

	// minmaxZ 数据，用于地形剔除
	// m_minmaxZData[mip][x][y] = Vector2(minZ, maxZ)
	std::vector<std::vector<std::vector<Vector2>>> m_minmaxZData;
};
