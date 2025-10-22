#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"

struct NXTerrainStreamingNode
{
	Int2 terrainID; // 地形ID
	Int2 positionWS; // 地形左下角XZ节点坐标（左手坐标系）
	uint32_t size; // 节点大小，一定是2的整数幂

	NXTerrainStreamingNode GetChildNode(int index) const
	{
		NXTerrainStreamingNode childNode;

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

struct NXTerrainStreamingNodeDescription
{
	NXTerrainStreamingNode data;

	// 记录上次更新的帧数；
	// 如果某个node长时间没有被访问到，则可以考虑卸载
	uint64_t lastUpdatedFrame; 
};

class NXCamera;
class NXScene;
class NXTerrainStreamingQuadTree
{
	static constexpr int s_maxNodeLevel = 5; // 最大节点层级 0~5 共6层
	static constexpr float s_distRanges[6] = { 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f }; // 这样写可以不在cpp再初始化一次 很方便

public:
	NXTerrainStreamingQuadTree() {}
	~NXTerrainStreamingQuadTree() {}

	void Init(NXScene* m_pScene);

	// 获取6档距离内的节点，输出一个list[6]；只要是当前档次距离能覆盖的，统统加入到预加载队列
	// nodeData总是临时的！所以无需释放
	void GetNodeDatas(std::vector<std::vector<NXTerrainStreamingNode>>& oNodeDataList);

	// 每帧更新
	void Update();

	void ProcessBatcher();

private:
	void GetNodeDatasInternal(std::vector<std::vector<NXTerrainStreamingNode>>& oNodeDataList, const NXTerrainStreamingNode& node);

private:
	std::filesystem::path m_terrainWorkingDir = "D:\\NixAssets\\terrainTest";

	// 每个地形是一个四叉树
	std::vector<NXTerrainStreamingNode> m_terrainRoots;

	// "已经加载"到Atlas的节点
	std::vector<NXTerrainStreamingNodeDescription> m_nodeDescArray;

	// "正在加载中"的节点
	std::vector<NXTerrainStreamingNodeDescription> m_loadingNodeDescArray;

	NXScene* m_pScene; 
};

