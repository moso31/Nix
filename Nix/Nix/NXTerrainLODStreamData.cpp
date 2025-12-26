#include "NXTerrainLODStreamData.h"
#include "NXTerrainLODStreamer.h"
#include "NXResourceManager.h"

void NXTerrainLODStreamData::Init(NXTerrainLODStreamer* pStreamer)
{
    // NodeDescArray_GPU 和CPU预分配相同的大小（默认1024）
	m_pTerrainNodeDescArray = new NXBuffer("Terrain NodeDescArray_GPU");
    m_pTerrainNodeDescArray->Create(sizeof(CBufferTerrainNodeDescription), pStreamer->s_nodeDescArrayInitialSize); 

	// NodeDescArray_CPU
    m_cbNodeDescArray.Recreate(pStreamer->s_nodeDescArrayInitialSize);
	m_nodeDescArray.resize(pStreamer->s_nodeDescArrayInitialSize);
	m_cbNodeDescUpdateIndices.Recreate(pStreamer->GetLoadTexGroupLimitEachFrame());

	// 纹理Atlas
	m_pHeightMapAtlas = NXManager_Tex->CreateUAVTexture2DArray("TerrainStreaming_HeightMapAtlas", DXGI_FORMAT_R16_FLOAT, s_atlasHeightMapSize, s_atlasHeightMapSize, s_atlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pSplatMapAtlas = NXManager_Tex->CreateUAVTexture2DArray("TerrainStreaming_SplatMapAtlas", DXGI_FORMAT_R8_UNORM, s_atlasSplatMapSize, s_atlasSplatMapSize, s_atlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 每帧待合并到Atlas的纹理列表
	m_pToAtlasHeights.resize(pStreamer->GetLoadTexGroupLimitEachFrame());
	m_pToAtlasSplats.resize(pStreamer->GetLoadTexGroupLimitEachFrame());
}

void NXTerrainLODStreamData::SetNodeDescArrayData(uint32_t index, const CBufferTerrainNodeDescription& data)
{
    if (index >= m_nodeDescArray.size()) return;
    m_nodeDescArray[index] = data;
	m_nodeDescUpdateIndices.push_back(index); // 本帧更新的索引也记录一下
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
