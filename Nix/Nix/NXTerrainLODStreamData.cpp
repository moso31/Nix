#include "NXTerrainLODStreamData.h"
#include "NXTerrainLODStreamer.h"
#include "NXResourceManager.h"

void NXTerrainLODStreamData::Init(NXTerrainLODStreamer* pStreamer)
{
    // NodeDescArray_GPU 和CPU预分配相同的大小（默认1024）
	m_pTerrainNodeDescArray = new NXBuffer("Terrain NodeDescArray_GPU");
    m_pTerrainNodeDescArray->Create(sizeof(CBufferTerrainNodeDescription), g_terrainStreamConfig.NodeDescArrayInitialSize); 

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
	m_pSector2NodeIDTexture = NXManager_Tex->CreateTexture2D("TerrainStreaming_Sector2NodeID", DXGI_FORMAT_R16_UINT, g_terrainStreamConfig.Sector2NodeIDTexSize, g_terrainStreamConfig.Sector2NodeIDTexSize, g_terrainStreamConfig.LODSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pSector2NodeIDTexture->SetViews(1, 0, 0, g_terrainStreamConfig.LODSize);
	m_pSector2NodeIDTexture->SetSRV(0);
	for (int i = 0; i < g_terrainStreamConfig.LODSize; i++)
	{
		m_pSector2NodeIDTexture->SetUAV(i, i); // 每个mip都需要UAV
	}
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
