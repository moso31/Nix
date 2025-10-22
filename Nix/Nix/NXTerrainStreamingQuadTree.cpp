#include "NXTerrainStreamingQuadTree.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"

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
    std::vector<NXTerrainStreamingNodeDescription> loadFromDiskNodeDescs;

    // 逆序，因为nodeLevel越大，对应node表示的精度越高；这里需要优先加载精度最高的
    int loadCount = 0;
    for (int i = s_maxNodeLevel; i >= 0; i--) 
    {
        for (auto& node : nodeLists[i])
        {
            auto it = std::find_if(m_nodeDescArray.begin(), m_nodeDescArray.end(), [&](const NXTerrainStreamingNodeDescription& nodeDesc) {
                return node.positionWS == nodeDesc.data.positionWS && node.size == nodeDesc.data.size;
                });

            auto itLoading = std::find_if(m_loadingNodeDescArray.begin(), m_loadingNodeDescArray.end(), [&](const NXTerrainStreamingNodeDescription& nodeDesc) {
				return node.positionWS == nodeDesc.data.positionWS && node.size == nodeDesc.data.size;
				});

            // 若节点已存在，或者正在加载
            if (it != m_nodeDescArray.end() || itLoading != m_loadingNodeDescArray.end())
            {
                // 仅需更新lastUpdate
                it->lastUpdatedFrame = NXGlobalApp::s_frameIndex.load();
            }
            else // 否则准备磁盘异步加载
            {
                loadCount++;

                NXTerrainStreamingNodeDescription newNodeDesc;
                newNodeDesc.data = node;
                newNodeDesc.lastUpdatedFrame = NXGlobalApp::s_frameIndex.load();

                loadFromDiskNodeDescs.push_back(newNodeDesc);
            }
        }
    }

    // 加入"加载中"队列
    m_loadingNodeDescArray.insert(m_loadingNodeDescArray.end(), loadFromDiskNodeDescs.begin(), loadFromDiskNodeDescs.end());

    // 开始加载需要的纹理。
    // 加载库本身就是异步的，这里直接调接口就OK
    for (auto& loadingDesc : loadFromDiskNodeDescs)
    {
        auto& task = loadingDesc.data;
        Int2 relativePos = task.positionWS - task.terrainID * g_terrainConfig.TerrainSize; // 地形块内相对左下角的位置

        // 加载高度图
        std::string strTerrId = std::to_string(task.terrainID.x) + "_" + std::to_string(task.terrainID.y);
        std::string strRaw = std::format("tile_{:02}_{:02}.raw", task.terrainID.x, task.terrainID.y);
        std::filesystem::path texHeightMapPath = m_terrainWorkingDir / strTerrId / strRaw;
        std::string strHeightMapName = "VT_HeightMap_" + strTerrId + "_tile" + std::to_string(relativePos.x) + "_" + std::to_string(relativePos.y);

        // 加载SplatMap图
        std::filesystem::path texSplatMapPath = m_terrainWorkingDir / "splatmap_uncompress.dds";
        std::string strSplatMapName = "VT_SplatMap_" + strTerrId + "_tile" + std::to_string(relativePos.x) + "_" + std::to_string(relativePos.y);

        //NXVTTexTask nextTask;
        //nextTask.pHeightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strHeightMapName, texHeightMapPath, task.tileRelativePos, task.tileSize + 1);
        //nextTask.pSplatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strSplatMapName, texSplatMapPath, task.tileRelativePos, task.tileSize + 1);
        //nextTask.TileWorldPos = task.terrainID * g_terrainConfig.TerrainSize + task.tileRelativePos + g_terrainConfig.MinTerrainPos;
        //nextTask.TileWorldSize = task.tileSize;
        //m_texTasks.push_back(nextTask);
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