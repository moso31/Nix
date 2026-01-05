#include "NXTerrainLODStreamer.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGlobalDefinitions.h"
#include "NXTerrainLODStreamConfigs.h"
#include "NXTerrainStreamingAsyncLoader.h"
#include "NXTerrainStreamingBatcher.h"
#include "DirectXTex.h"

NXTerrainLODStreamer::NXTerrainLODStreamer() :
    m_asyncLoader(new NXTerrainStreamingAsyncLoader())
{
	m_streamData.Init(this);

    // nodeDescArray长度固定不变
    m_nodeDescArrayInternal.resize(g_terrainStreamConfig.NodeDescArrayInitialSize);
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
    // 如果调试模式下暂停异步加载，则跳过本次更新
    if (g_terrainStreamDebug.bPauseAsyncLoading)
        return;

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
    for (int i = g_terrainStreamConfig.MaxNodeLevel; i >= 0; i--) 
    {
        for (auto& node : nodeLists[i])
        {
            // 辅助快查map提速，原本直接对 m_nodeDescArrayInternal做find_if 开销有点高了
            TerrainNodeKey key;
            key.positionWS = node.positionWS;
            key.size = node.size;
            auto itKey = m_keyToNodeMap.find(key);
            if (itKey != m_keyToNodeMap.end())
            {
                // 检查缓存中是否已经具有位置和大小相同的节点（正在加载中的也算）
                // 若节点已存在，或者正在加载
                if (m_nodeDescArrayInternal[itKey->second].isLoading || m_nodeDescArrayInternal[itKey->second].isValid)
                {
                    // 仅需更新lastUpdate
                    m_nodeDescArrayInternal[itKey->second].lastUpdatedFrame = NXGlobalApp::s_frameIndex.load();
                    continue;
                }
            }

            // 若节点不存在，准备从磁盘异步加载，从nodeDesc中选一个空闲的或最久未使用的
            {
                uint64_t oldestFrame = UINT64_MAX;
                uint32_t oldestIndex = -1;
                uint32_t selectedIndex = -1;

                for (uint32_t descIndex = 0; descIndex < m_nodeDescArrayInternal.size(); descIndex++)
                {
                    // 优先取空闲（!isValid）；正在加载的（isLoading）不考虑
                    auto& nodeDesc = m_nodeDescArrayInternal[descIndex];
                    if (!nodeDesc.isValid && !nodeDesc.isLoading)
                    {
                        selectedIndex = descIndex;
                        break;
                    }

					// 若没有空闲，则找最久未使用的
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
                    auto& selectedNodeDesc = m_nodeDescArrayInternal[selectedIndex];
                    if (oldestIndex != -1)
                    {
                        // 如果要替换最久未使用的节点 记录替换前的数据
                        selectedNodeDesc.oldData = selectedNodeDesc.data; 
                        selectedNodeDesc.removeOldData = true;

                        // 辅助快查map更新
                        TerrainNodeKey oldKey;
                        oldKey.positionWS = selectedNodeDesc.oldData.positionWS;
                        oldKey.size = selectedNodeDesc.oldData.size;
                        m_keyToNodeMap.erase(oldKey);
                    }
                    else
                    {
                        selectedNodeDesc.removeOldData = false;
                    }

                    // 辅助快查map更新
                    m_keyToNodeMap[key] = selectedIndex;

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
        // 对地形留点文本说明 方便以后查bug：
        // 下面这些数据都验证过了，除非后面又改相关工作流，不然不用怀疑！
        //      整个世界是一个 - 8192, -8192 ~8192, 8192 的矩形，每个大的地形块大小为2048, 2048
        //      Nix 使用 Y上X右Z前 的左手坐标系。
        //      下面我们说坐标时，如无特殊说明，统一使用每个地形的左下角坐标。
        //      每个地形有一个编号。俯视角看，从左上到右下，第一行，0_0, 0_1, 0_2, ..., 0_7.第二行，1_0, 1_1....以此类推。
        //      作为参考，左下角的地形为7_0，其地形左下角坐标为（-8192, -8192）。
        //      每个地形与缓存若干patch纹理。通常名为
        //      \{Nix资源文件夹}\Terrain\{地形行}_{ 地形列 }\sub\hmap\{patch纹理实际大小}_{ patch行 }_{ patch列 }.dds
        //      大致就以上这些。

		static Int2 minTerrainID = g_terrainConfig.MinTerrainPos / g_terrainConfig.TerrainSize;

        auto& data = m_nodeDescArrayInternal[loadingDesc].data;
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
        //printf("loading: data.size = %d, mip = %d; posWS = %d %d; minMaxZ: %f %f\n", data.size, bx, by, bz, minmaxZ.x, minmaxZ.y);

		Int2 strID = data.terrainID - minTerrainID;
        int row = 7 - strID.y;  // 换算了下应该是Flip V
        int col = strID.x;
        std::string strTerrId = std::to_string(row) + "_" + std::to_string(col);
        std::string strTerrSubID = std::to_string(realSize) + "_" + std::to_string(relativePosID.x) + "_" + std::to_string(relativePosID.y);

        TerrainStreamingLoadRequest task;
        // 新的位置、大小、minmaxz
		task.positionWS = data.positionWS;
        task.size = realSize;
		task.nodeDescArrayIndex = loadingDesc;
		task.minMaxZ = minmaxZ;

        // 如果有旧的信息task也记一下，同步到GPU，有删除操作依赖
        if (m_nodeDescArrayInternal[loadingDesc].removeOldData)
        {
            auto& oldData = m_nodeDescArrayInternal[loadingDesc].oldData;
            task.replacePositionWS = oldData.positionWS;
            task.replaceSize = oldData.size;
        }
        
        task.heightMap.path = m_terrainWorkingDir / strTerrId / "sub" / "hmap" / (strTerrSubID + ".dds");
        task.heightMap.name = "Terrain_HeightMap_" + strTerrId + "_tile_" + strTerrSubID;

        task.splatMap.path = m_terrainWorkingDir / strTerrId / "sub" / "splat" / (strTerrSubID + ".dds");
        task.splatMap.name = "Terrain_SplatMap_" + strTerrId + "_tile_" + strTerrSubID;

        m_asyncLoader->AddTask(task);
        //printf("%d %s\n", task.nodeDescArrayIndex, task.splatMap.path.string().c_str());
    }
}

void NXTerrainLODStreamer::UpdateAsyncLoader()
{
    // 如果调试模式下暂停异步加载，则跳过本次更新
    if (g_terrainStreamDebug.bPauseAsyncLoading)
        return;

	// 更新异步加载器
    m_asyncLoader->Update();
}

void NXTerrainLODStreamer::ProcessCompletedStreamingTask()
{
    m_streamData.ClearNodeDescUpdateIndices(); // 更新索引每帧清空

    auto& completeTasks = m_asyncLoader->ConsumeCompletedTasks();
    for (int i = 0; i < completeTasks.size(); i++)
    {
        auto& task = completeTasks[i];

        m_nodeDescArrayInternal[task.nodeDescArrayIndex].isLoading = false;
        m_nodeDescArrayInternal[task.nodeDescArrayIndex].isValid = true;

        // 要更新的索引、数据；还有此索引上残留的旧数据(replaced)
        CBufferTerrainNodeDescription data;
		data.minmaxZ = task.minMaxZ;
		data.positionWS = task.positionWS;
		data.size = task.size;
        m_streamData.SetNodeDescArrayData(task.nodeDescArrayIndex, data, task.replacePositionWS, task.replaceSize);

        m_streamData.SetToAtlasHeightTexture(i, task.pHeightMap);
        m_streamData.SetToAtlasSplatTexture(i, task.pSplatMap);
    }

    m_streamData.UpdateCBNodeDescArray();
}

uint32_t NXTerrainLODStreamer::GetLoadTexGroupLimitEachFrame()
{
    return g_terrainStreamConfig.MaxComputeLimit;
}

void NXTerrainLODStreamer::GetNodeDatasInternal(std::vector<std::vector<NXTerrainLODQuadTreeNode>>& oNodeDataList, const NXTerrainLODQuadTreeNode& node)
{
    // 从根节点向下遍历四叉树，规则：
    // - 先判断节点是否在当前LOD距离内
    // - 如果节点在当前LOD距离内，递归拿四个子节点并进入下一级LOD；否则中止递归

    uint32_t nodeLevel = node.GetLevel();
    float range = g_terrainStreamConfig.DistRanges[g_terrainStreamConfig.MaxNodeLevel - nodeLevel];

    auto pCamera = m_pScene->GetMainCamera();
    Rect2D nodeRect(Vector2((float)node.positionWS.x, (float)node.positionWS.y), (float)node.size);
    Circle2D camCircle(pCamera->GetTranslation().GetXZ(), range);

    // 如果相机范围-节点 相交
    if (nodeRect.Intersect(camCircle))
    {
        oNodeDataList[nodeLevel].push_back(node);

        // 继续递归子节点
        if (nodeLevel + 1 <= g_terrainStreamConfig.MaxNodeLevel)
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

    // 这是一个 2D Array 纹理：
    // - mipLevels 对应不同的四叉树 LOD 层级
    // - arraySize = 64（8x8 地形块），需要平摊到全局 x/z 坐标
    // - slice 排列：从左上到右下，先行后列。slice 0 = 0_0（左上角），slice 1 = 0_1，...，slice 8 = 1_0
    // 
    // mmz.dds 原始数据是 patcher 粒度（mip0: 256x256 per slice，每 patcher 8x8）
    // 流式加载地形上线后，需要合并到 node 粒度（mip0: 32x32 per slice，每 node 64x64）
    // 每 8x8 个 patcher 合并成 1 个 node，取这 64 个 patcher 的 min/max
    // 最终数据结构：m_minmaxZData[mip][globalNodeX][globalNodeZ]
    size_t arraySize = metadata.arraySize;
    size_t mipCount = metadata.mipLevels;

    printf("LoadMinmaxZData: arraySize=%zu, mipLevels=%zu, baseWidth=%zu, baseHeight=%zu\n",
           arraySize, mipCount, metadata.width, metadata.height);

    m_minmaxZData.resize(mipCount);

    for (size_t mip = 0; mip < mipCount; mip++)
    {
        // 获取第一个 slice 来确定这个 mip 级别的尺寸
        const Image* firstImg = scratchImage.GetImage(mip, 0, 0);
        if (!firstImg)
        {
            printf("LoadMinmaxZData: 获取 mip %zu slice 0 失败\n", mip);
            continue;
        }

        size_t sliceWidth = firstImg->width;   // patcher 粒度的宽度
        size_t sliceHeight = firstImg->height; // patcher 粒度的高度

        // 计算 node 粒度的尺寸（每 8x8 patcher 合并为 1 个 node）
        // 对于低 mip 级别，slice 尺寸可能小于 8，此时不需要合并
        size_t mergeFactorX = std::min(sliceWidth, (size_t)8);
        size_t mergeFactorZ = std::min(sliceHeight, (size_t)8);
        size_t nodeSliceWidth = (sliceWidth + mergeFactorX - 1) / mergeFactorX;
        size_t nodeSliceHeight = (sliceHeight + mergeFactorZ - 1) / mergeFactorZ;

        // 全局 node 数量 = node slice 大小 * 8（8x8 地形块）
        size_t globalNodeWidth = nodeSliceWidth * 8;
        size_t globalNodeHeight = nodeSliceHeight * 8;

        m_minmaxZData[mip].resize(globalNodeWidth);
        for (size_t x = 0; x < globalNodeWidth; x++)
        {
            m_minmaxZData[mip][x].resize(globalNodeHeight);
        }

        // 遍历每个 slice
        for (size_t slice = 0; slice < arraySize; slice++)
        {
            // slice 排列：从左上到右下，先行后列
            // slice 0 = 0_0（左上角），slice 1 = 0_1，...，slice 8 = 1_0
            size_t sliceRow = slice / 8;  // 0-7，0是上面（z 最大）
            size_t sliceCol = slice % 8;  // 0-7，0是左边（x 最小）

            const Image* img = scratchImage.GetImage(mip, slice, 0);
            if (!img)
            {
                printf("LoadMinmaxZData: 获取 mip %zu slice %zu 失败\n", mip, slice);
                continue;
            }

            // 计算这个 slice 在全局 node 数组中的起始位置
            // sliceCol 直接映射到 x
            // sliceRow 需要翻转：row 0（上面）对应全局 z 最大
            size_t globalNodeStartX = sliceCol * nodeSliceWidth;
            size_t globalNodeStartZ = (7 - sliceRow) * nodeSliceHeight;

            const uint8_t* pixels = img->pixels;
            size_t rowPitch = img->rowPitch;

            // 遍历每个 node（每个 node 对应 8x8 个 patcher）
            for (size_t nodeY = 0; nodeY < nodeSliceHeight; nodeY++)
            {
                for (size_t nodeX = 0; nodeX < nodeSliceWidth; nodeX++)
                {
                    float nodeMinZ = FLT_MAX;
                    float nodeMaxZ = -FLT_MAX;

                    // 遍历这个 node 内的所有 patcher（最多 8x8）
                    size_t patcherStartX = nodeX * mergeFactorX;
                    size_t patcherStartY = nodeY * mergeFactorZ;
                    size_t patcherEndX = std::min(patcherStartX + mergeFactorX, sliceWidth);
                    size_t patcherEndY = std::min(patcherStartY + mergeFactorZ, sliceHeight);

                    for (size_t py = patcherStartY; py < patcherEndY; py++)
                    {
                        const uint8_t* row = pixels + py * rowPitch;
                        for (size_t px = patcherStartX; px < patcherEndX; px++)
                        {
                            const float* pixelData = reinterpret_cast<const float*>(row + px * sizeof(float) * 2);
                            float minZ = pixelData[0];
                            float maxZ = pixelData[1];

                            nodeMinZ = std::min(nodeMinZ, minZ);
                            nodeMaxZ = std::max(nodeMaxZ, maxZ);
                        }
                    }

                    // 图像 y=0 在上面，翻转以适配世界坐标
                    size_t localNodeZ = nodeSliceHeight - 1 - nodeY;

                    size_t gx = globalNodeStartX + nodeX;
                    size_t gz = globalNodeStartZ + localNodeZ;

                    m_minmaxZData[mip][gx][gz] = Vector2(nodeMinZ, nodeMaxZ);
                }
            }
        }
    }
}
