#include "NXTerrainStreamingQuadTree.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXTerrain.h"

void NXTerrainStreamingQuadTree::Init(NXScene* pScene)
{
    // 遍历场景所有地形，拿地形位置，由此形成四叉树根节点
    m_pScene = pScene;
    auto& pTerrains = m_pScene->GetTerrains();

    for (auto& [terraID, pTerra] : pTerrains)
    {
        NXTerrainStreamingNode terrainRoot;
        terrainRoot.position = Int2(terraID.x, terraID.y) * s_terrainSize; // 地形左下角位置
        terrainRoot.size = s_terrainSize;

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

void NXTerrainStreamingQuadTree::GetNodeDatasInternal(std::vector<std::vector<NXTerrainStreamingNode>>& oNodeDataList, const NXTerrainStreamingNode& node)
{
    // 从根节点向下遍历四叉树，规则：
    // - 先判断节点是否在当前LOD距离内
    // - 如果节点在当前LOD距离内，递归拿四个子节点并进入下一级LOD；否则中止递归

    uint32_t nodeLevel = node.GetLevel();
    float range = s_distRanges[nodeLevel];

    auto pCamera = m_pScene->GetMainCamera();
    Rect2D nodeRect(Vector2(node.position.x, node.position.y), node.size);
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