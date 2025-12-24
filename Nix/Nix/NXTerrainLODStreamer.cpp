#include "NXTerrainLODStreamer.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXTerrainStreamingAsyncLoader.h"
#include "NXTerrainStreamingBatcher.h"
#include "DirectXTex.h"

NXTerrainLODStreamer::NXTerrainLODStreamer() :
    m_asyncLoader(new NXTerrainStreamingAsyncLoader())
{
    // nodeDescArray长度固定不变
    m_nodeDescArray.resize(s_nodeDescArrayInitialSize);
}

NXTerrainLODStreamer::~NXTerrainLODStreamer()
{
    delete m_asyncLoader;
}

void NXTerrainLODStreamer::Init(NXScene* pScene)
{
    // 遍历场景所有地形，拿地形位置，由此形成四叉树根节点
    m_pScene = pScene;
    auto& pTerrains = m_pScene->GetTerrains();

    for (auto& [terraID, pTerra] : pTerrains)
    {
        NXTerrainLODQuadTreeNode terrainRoot;
        terrainRoot.terrainID = Int2(terraID.x, terraID.y);
        terrainRoot.positionWS = Int2(terraID.x, terraID.y) * g_terrainConfig.TerrainSize; // 地形左下角位置
        terrainRoot.size = g_terrainConfig.TerrainSize;

        m_terrainRoots.push_back(terrainRoot);
    }

    LoadMinmaxZData();
}

void NXTerrainLODStreamer::Update()
{
    // 获取6档LOD覆盖的所有四叉树节点
    std::vector<std::vector<NXTerrainLODQuadTreeNode>> nodeLists(6);

    // 遍历所有地形，按6档LOD距离加载node
    for (auto& terrainRoot : m_terrainRoots)
    {
        GetNodeDatasInternal(nodeLists, terrainRoot);
    }

    // 统计本帧需要加载的nodeId
    std::vector<uint32_t> nextLoadingNodeDescIndices;

    // 逆序，因为nodeLevel越大，对应node表示的精度越高；这里需要优先加载精度最高的
    int loadCount = 0;
    for (int i = s_maxNodeLevel; i >= 0; i--) 
    {
        for (auto& node : nodeLists[i])
        {
            auto it = std::find_if(m_nodeDescArray.begin(), m_nodeDescArray.end(), [&](const NXTerrainLODQuadTreeNodeDescription& nodeDesc) {
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
		static Int2 minTerrainID = g_terrainConfig.MinTerrainPos / g_terrainConfig.TerrainSize;

        auto& data = m_nodeDescArray[loadingDesc].data;
        Int2 relativePos = data.positionWS - data.terrainID * g_terrainConfig.TerrainSize; // 地形块内相对左下角的位置
        relativePos.y = 2048 - relativePos.y; // flip Y
        Int2 relativePosID = relativePos / data.size;
        relativePosID.y--; 
        int realSize = data.size + 1;

        // 获取minmaxZ
        int bitOffsetXY = std::countr_zero(data.size); // data.size一定是2的整数倍并且>=64
        int bitOffsetMip = bitOffsetXY - std::countr_zero(64u);
        Int2 offsetPosWS = data.positionWS - g_terrainConfig.MinTerrainPos;
        int bx = bitOffsetMip;
		int by = (offsetPosWS.x >> bitOffsetXY);
		int bz = (offsetPosWS.y >> bitOffsetXY);
		Vector2 minmaxZ = m_minmaxZData[bx][by][bz];

		Int2 strID = data.terrainID - minTerrainID;
        std::string strTerrId = std::to_string(strID.x) + "_" + std::to_string(strID.y);
        std::string strTerrSubID = std::to_string(realSize) + "_" + std::to_string(relativePosID.x) + "_" + std::to_string(relativePosID.y);

        TerrainStreamingLoadRequest task;
        task.terrainID = data.terrainID;
        task.relativePos = relativePos;
        task.size = realSize;
		task.nodeDescArrayIndex = loadingDesc;
		task.minMaxZ = minmaxZ;
        
        // 使用正确的路径分隔符，确保与TextureMaker的路径格式一致
        task.heightMap.path = m_terrainWorkingDir / strTerrId / "sub" / "hmap" / (strTerrSubID + ".dds");
        task.heightMap.name = "Terrain_HeightMap_" + strTerrId + "_tile_" + strTerrSubID;

        task.splatMap.path = m_terrainWorkingDir / strTerrId / "sub" / "splat" / (strTerrSubID + ".dds");
        task.splatMap.name = "Terrain_SplatMap_" + strTerrId + "_tile_" + strTerrSubID;

        m_asyncLoader->AddTask(task);
    }
}

void NXTerrainLODStreamer::UpdateAsyncLoader()
{
	// 更新异步加载器
    m_asyncLoader->Update();
}

void NXTerrainLODStreamer::ProcessCompletedStreamingTask()
{
    for (auto& task : m_asyncLoader->ConsumeCompletedTasks())
    {
        printf("%s\n", task.pSplatMap->GetFilePath().string().c_str());
        //NXTerrainStreamingBatcher::GetInstance()->PushCompletedTask(task);
    }
}

void NXTerrainLODStreamer::GetNodeDatasInternal(std::vector<std::vector<NXTerrainLODQuadTreeNode>>& oNodeDataList, const NXTerrainLODQuadTreeNode& node)
{
    // 从根节点向下遍历四叉树，规则：
    // - 先判断节点是否在当前LOD距离内
    // - 如果节点在当前LOD距离内，递归拿四个子节点并进入下一级LOD；否则中止递归

    uint32_t nodeLevel = node.GetLevel();
    float range = s_distRanges[s_maxNodeLevel - nodeLevel];

    auto pCamera = m_pScene->GetMainCamera();
    Rect2D nodeRect(Vector2((float)node.positionWS.x, (float)node.positionWS.y), (float)node.size);
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

void NXTerrainLODStreamer::LoadMinmaxZData()
{
    using namespace DirectX;

    std::filesystem::path mmzPath = m_terrainWorkingDir / "mmz.dds";

    TexMetadata metadata;
    ScratchImage scratchImage;
    HRESULT hr = LoadFromDDSFile(mmzPath.wstring().c_str(), DDS_FLAGS_NONE, &metadata, scratchImage);

    if (FAILED(hr))
    {
        printf("LoadMinmaxZData: 加载失败 %s\n", mmzPath.string().c_str());
        return;
    }

    // 验证格式是否为 R32G32_FLOAT
    if (metadata.format != DXGI_FORMAT_R32G32_FLOAT)
    {
        printf("LoadMinmaxZData: 格式不匹配，期望 R32G32_FLOAT，实际 %d\n", (int)metadata.format);
        return;
    }

    // 初始化 m_minmaxZData，维度为 [mipCount][width][height]
    size_t mipCount = metadata.mipLevels;
    m_minmaxZData.resize(mipCount);

    for (size_t mip = 0; mip < mipCount; mip++)
    {
        const Image* img = scratchImage.GetImage(mip, 0, 0);
        if (!img)
        {
            printf("LoadMinmaxZData: 获取 mip %zu 失败\n", mip);
            continue;
        }

        size_t mipWidth = img->width;
        size_t mipHeight = img->height;

        m_minmaxZData[mip].resize(mipWidth);
        for (size_t x = 0; x < mipWidth; x++)
        {
            m_minmaxZData[mip][x].resize(mipHeight);
        }

        // 读取像素数据，R32G32_FLOAT 每像素 8 字节
        const uint8_t* pixels = img->pixels;
        size_t rowPitch = img->rowPitch;

        for (size_t y = 0; y < mipHeight; y++)
        {
            const uint8_t* row = pixels + y * rowPitch;
            for (size_t x = 0; x < mipWidth; x++)
            {
                const float* pixelData = reinterpret_cast<const float*>(row + x * sizeof(float) * 2);
                float minZ = pixelData[0];
                float maxZ = pixelData[1];

                // y 翻转以适配世界坐标
                size_t flippedY = mipHeight - 1 - y;
                m_minmaxZData[mip][x][flippedY] = Vector2(minZ, maxZ);
            }
        }
    }
}
