#include "NXTerrainStreamingQuadTree.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXTerrainStreamingAsyncLoader.h"

NXTerrainStreamingQuadTree::NXTerrainStreamingQuadTree() :
    m_asyncLoader(new NXTerrainStreamingAsyncLoader())
{
    // nodeDescArray长度固定不变
    m_nodeDescArray.resize(s_nodeDescArrayInitialSize);
}

NXTerrainStreamingQuadTree::~NXTerrainStreamingQuadTree()
{
    delete m_asyncLoader;
}

void NXTerrainStreamingQuadTree::Init(NXScene* pScene)
{
    // 遍历场景所有地形，拿地形位置，由此形成四叉树根节点
    m_pScene = pScene;
    auto& pTerrains = m_pScene->GetTerrains();

    for (auto& [terraID, pTerra] : pTerrains)
    {
        NXTerrainStreamingNode terrainRoot;
        terrainRoot.terrainID = Int2(terraID.x, terraID.y);
        terrainRoot.positionWS = Int2(terraID.x, terraID.y) * g_terrainConfig.TerrainSize; // 地形左下角位置
        terrainRoot.size = g_terrainConfig.TerrainSize;

        m_terrainRoots.push_back(terrainRoot);
    }
}

void NXTerrainStreamingQuadTree::GetNodeDatas(std::vector<std::vector<NXTerrainStreamingNode>>& oNodeDataList)
{
	oNodeDataList.clear();
	oNodeDataList.resize(6);

    // 遍历所有地形
    for (auto& terrainRoot : m_terrainRoots)
    {
        GetNodeDatasInternal(oNodeDataList, terrainRoot);
    }
}

void NXTerrainStreamingQuadTree::Update()
{
    std::vector<std::vector<NXTerrainStreamingNode>> nodeLists;
    GetNodeDatas(nodeLists);

    // 统计本帧需要加载的node
    std::vector<uint32_t> nextLoadingNodeDescIndices;

    // 逆序，因为nodeLevel越大，对应node表示的精度越高；这里需要优先加载精度最高的
    int loadCount = 0;
    for (int i = s_maxNodeLevel; i >= 0; i--) 
    {
        for (auto& node : nodeLists[i])
        {
            auto it = std::find_if(m_nodeDescArray.begin(), m_nodeDescArray.end(), [&](const NXTerrainStreamingNodeDescription& nodeDesc) {
                // 检查缓存中是否已经具有位置和大小相同，并且是有效（或正在加载中的）节点
                return node.positionWS == nodeDesc.data.positionWS && node.size == nodeDesc.data.size && (nodeDesc.isLoading || nodeDesc.isValid);
                });

            // 若节点已存在，或者正在加载
            if (it != m_nodeDescArray.end())
            {
                // 仅需更新lastUpdate
                it->lastUpdatedFrame = NXGlobalApp::s_frameIndex.load();
            }
            else // 否则准备磁盘异步加载，从nodeDesc中选一个空闲的或最久未使用的
            {
                loadCount++;

                uint64_t oldestFrame = UINT64_MAX;
                uint32_t oldestIndex = -1;
                uint32_t selectedIndex = -1;

                // 首先尝试找空闲节点
                for (uint32_t descIndex = 0; descIndex < m_nodeDescArray.size(); descIndex++)
                {
                    auto& nodeDesc = m_nodeDescArray[descIndex];
                    if (!nodeDesc.isValid && !nodeDesc.isLoading)
                    {
                        selectedIndex = descIndex;
                        break;
                    }

                    if (nodeDesc.isValid && nodeDesc.lastUpdatedFrame < oldestFrame)
                    { 
                        oldestFrame = nodeDesc.lastUpdatedFrame;
                        oldestIndex = descIndex;
                    }
                }

                // 如果没有空闲节点，找到最久未使用的节点
                if (selectedIndex == -1 && oldestIndex != -1)
                {
                    selectedIndex = oldestIndex;
                }
                
                if (selectedIndex != -1)
                {
                    auto& selectedNodeDesc = m_nodeDescArray[selectedIndex];
                    selectedNodeDesc.isLoading = true;
                    selectedNodeDesc.isValid = false;
                    selectedNodeDesc.data = node;
                    selectedNodeDesc.lastUpdatedFrame = NXGlobalApp::s_frameIndex.load();
                    nextLoadingNodeDescIndices.push_back(selectedIndex);
                }
            }
        }
    }

    // 开始加载需要的纹理。
    // 加载库本身就是异步的，这里直接调接口就OK
    for (auto& loadingDesc : nextLoadingNodeDescIndices)
    {
        auto& data = m_nodeDescArray[loadingDesc].data;
        Int2 relativePos = data.positionWS - data.terrainID * g_terrainConfig.TerrainSize; // 地形块内相对左下角的位置
        Int2 relativePosID = relativePos / g_terrainConfig.SectorSize;

        std::string strTerrId = std::to_string(data.terrainID.x) + "_" + std::to_string(data.terrainID.y);
        std::string strTerrSubID = std::to_string(data.size) + "_" + std::to_string(relativePosID.x) + "_" + std::to_string(relativePosID.y);

        TerrainStreamingLoadRequest task;
        task.terrainID = data.terrainID;
        task.relativePosID = relativePosID;
        task.size = data.size;
        task.heightMap.path = m_terrainWorkingDir / strTerrId / "sub\\hmap\\" / strTerrSubID + ".dds";
        task.heightMap.name = "Terrain_HeightMap_" + strTerrId + "_tile_" + strTerrSubID;

        task.splatMap.path = m_terrainWorkingDir / strTerrId / "sub\\splat\\" / strTerrSubID + ".dds";
        task.splatMap.name = "Terrain_SplatMap_" + strTerrId + "_tile_" + strTerrSubID;

        m_asyncLoader->AddTask(task);
    }
}

void NXTerrainStreamingQuadTree::ProcessBatcher()
{
}

void NXTerrainStreamingQuadTree::GetNodeDatasInternal(std::vector<std::vector<NXTerrainStreamingNode>>& oNodeDataList, const NXTerrainStreamingNode& node)
{
    // 从根节点向下遍历四叉树，规则：
    // - 先判断节点是否在当前LOD距离内
    // - 如果节点在当前LOD距离内，递归拿四个子节点并进入下一级LOD；否则中止递归

    uint32_t nodeLevel = node.GetLevel();
    float range = s_distRanges[nodeLevel];

    auto pCamera = m_pScene->GetMainCamera();
    Rect2D nodeRect(Vector2(node.positionWS.x, node.positionWS.y), node.size);
    Circle2D camCircle(pCamera->GetTranslation().GetXZ(), range);

    // 如果相机范围-节点 相交
    if (nodeRect.Intersect(camCircle))
    {
        oNodeDataList[nodeLevel].push_back(node);

        // 继续递归子节点
        if (nodeLevel + 1 <= s_maxNodeLevel)
        {
            for (int i = 0; i < 4; i++)
            {
                auto childNode = node.GetChildNode(i);
                GetNodeDatasInternal(oNodeDataList, childNode);
            }
        }
    }
}

void NXTerrainStreamingQuadTree::PickANode()
{
	for (auto& nodeDesc : m_nodeDescArray)
	{
        // 找到一个未使用的节点
		if (!nodeDesc.isValid && !nodeDesc.isLoading)
		{
			nodeDesc.isLoading = true;
			return;
		}
	}
}
