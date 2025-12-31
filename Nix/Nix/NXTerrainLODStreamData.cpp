#include "NXTerrainLODStreamData.h"
#include "NXTerrainLODStreamer.h"
#include "NXResourceManager.h"
#include "NXCamera.h"

void NXTerrainLODStreamData::Init(NXTerrainLODStreamer* pStreamer)
{
	// NodeDescArray_CPU
    m_cbNodeDescArray.Recreate(g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_nodeDescArray.resize(g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_cbNodeDescUpdateIndices.Recreate(g_terrainStreamConfig.MaxComputeLimit);

	// 纹理Atlas
	m_pHeightMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_HeightMapAtlas", DXGI_FORMAT_R16_UNORM, g_terrainStreamConfig.AtlasHeightMapSize, g_terrainStreamConfig.AtlasHeightMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pSplatMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_SplatMapAtlas", DXGI_FORMAT_R8_UNORM, g_terrainStreamConfig.AtlasSplatMapSize, g_terrainStreamConfig.AtlasSplatMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 每帧待合并到Atlas的纹理列表
	m_pToAtlasHeights.resize(g_terrainStreamConfig.MaxComputeLimit);
	m_pToAtlasSplats.resize(g_terrainStreamConfig.MaxComputeLimit);

	// 记录各sector的nodeID
	uint32_t mips = g_terrainStreamConfig.LODSize; // 6
	m_pSector2NodeIDTexture = NXManager_Tex->CreateTexture2D("TerrainStreaming_Sector2NodeID", DXGI_FORMAT_R16_UINT, g_terrainStreamConfig.Sector2NodeIDTexSize, g_terrainStreamConfig.Sector2NodeIDTexSize, mips, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pSector2NodeIDTexture->SetViews(1, 0, 0, mips);
	m_pSector2NodeIDTexture->SetSRV(0);
	for (int i = 0; i < mips; i++)
	{
		m_pSector2NodeIDTexture->SetUAV(i, i); // 每个mip都需要UAV
	}

	// gpu-driven ping-pong
	m_pingpongNodesA = new NXBuffer("Terrain Ping-pong NodeId Array A");
	m_pingpongNodesA->Create(sizeof(int), g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_pingpongNodesB = new NXBuffer("Terrain Ping-pong NodeId Array B");
	m_pingpongNodesB->Create(sizeof(int), g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_pingpongNodesFinal = new NXBuffer("Terrain Ping-pong NodeId Array Final");
	m_pingpongNodesFinal->Create(sizeof(int), g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_pingpongIndirectArgs = new NXBuffer("Terrain Ping-pong Indirect Args");
	m_pingpongIndirectArgs->Create(sizeof(int) * 3, 1);
	int a[3] = { 1, 1, 1 };
	m_pingpongIndirectArgs->SetAll(a, 1);

	// culling data 初始化时部分数据更新
	for (int i = 0; i < g_terrainStreamConfig.LODSize; i++)
	{
		int val = g_terrainStreamConfig.LODSize - i - 1;
		m_cbCullingData[i].m_currentLodDist = g_terrainStreamConfig.DistRanges[val];
		m_cbCullingData[i].m_currentMip = val;
	}

	// gpu-driven patcher
	m_patcherBuffer = new NXBuffer("GPU Terrain Patcher Buffer"); 
	m_patcherBuffer->Create(sizeof(TerrainPatchParam), 1024); // 不要用 g_terrainStreamConfig.NodeDescArrayInitialSize，二者含义有区别

	m_patcherDrawArgs = new NXBuffer("GPU Terrain Draw Index Indirect Args");
	m_patcherDrawArgs->Create(sizeof(int) * 5, 1);
	int a2[5] = { 0,0,0,0,0 };
	m_patcherDrawArgs->SetAll(a2, 1);
}

void NXTerrainLODStreamData::SetNodeDescArrayData(uint32_t index, const CBufferTerrainNodeDescription& data, const Int2& replacedPosWS, int replacedSize)
{
    if (index >= m_nodeDescArray.size()) return;
    m_nodeDescArray[index] = data;

	CBufferTerrainNodeDescUpdateInfo info;
	info.newIndex = index;
	info.replacePosWS = replacedPosWS;
	info.replaceSize = replacedSize;
	m_nodeDescUpdateIndices.push_back(info); // 本帧更新的索引也记录一下
}

void NXTerrainLODStreamData::UpdateCBNodeDescArray()
{
	// 图省事直接每帧全更新了，实际上每帧最多只更新几个。
    // 就20KB，应该还好
    m_cbNodeDescArray.Update(m_nodeDescArray);
	m_cbNodeDescUpdateIndices.Update(m_nodeDescUpdateIndices); 
}

void NXTerrainLODStreamData::ClearNodeDescUpdateIndices()
{
	m_nodeDescUpdateIndices.clear();
}

void NXTerrainLODStreamData::UpdateCullingData(NXCamera* pCamera)
{
	for (int i = 0; i < g_terrainStreamConfig.LODSize; i++)
	{
		m_cbCullingData[i].m_cameraPos = pCamera->GetTranslation();
		m_cbCulling[i].Update(m_cbCullingData[i]);
	}
}
