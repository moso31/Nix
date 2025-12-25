#include "NXTerrainStreamingBatcher.h"
#include "NXGlobalDefinitions.h"
#include "NXAllocatorManager.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXSamplerManager.h"
#include "NXTerrainCommon.h"

void NXTerrainStreamingBatcher::Init()
{
	static const int kAtlasHeightMapSize = 65;	// HeightMap: 65x65x100
	static const int kAtlasSplatMapSize = 65;	// SplatMap: 65x65x100
	static const int kAtlasLayerCount = 100;	// atlas tex2darray 数组长度
	static const int kSector2NodeIDTexSize = 256;

	m_pHeightMapAtlas = NXResourceManager::GetInstance()->GetTextureManager()->CreateUAVTexture2DArray("TerrainStreaming_HeightMapAtlas", DXGI_FORMAT_R16_FLOAT, kAtlasHeightMapSize, kAtlasHeightMapSize, kAtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pSplatMapAtlas = NXResourceManager::GetInstance()->GetTextureManager()->CreateUAVTexture2DArray("TerrainStreaming_SplatMapAtlas", DXGI_FORMAT_R8G8B8A8_UNORM, kAtlasSplatMapSize, kAtlasSplatMapSize, kAtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_pSector2NodeIDTexture = NXManager_Tex->CreateUAVTexture("TerrainStreaming_Sector2NodeID", DXGI_FORMAT_R16_UINT, kSector2NodeIDTexSize, kSector2NodeIDTexSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_pNodeDescriptionsArray = new NXBuffer("TerrainStreaming_NodeDescriptionsGPU");
	m_pNodeDescriptionsArray->Create(sizeof(NXTerrainStreamNodeDescriptionGPU), kAtlasLayerCount);

	m_batchingHeightMap.resize(MAX_PROCESSING_TEXTURE_PACKAGE);
	m_batchingSplatMap.resize(MAX_PROCESSING_TEXTURE_PACKAGE);
}

void NXTerrainStreamingBatcher::PushCompletedTask(const NXTerrainStreamingLoadTextureResult& task)
{
	m_completedTasks.push_back(task);

	// 确保 m_batchingNodeDescData 与 m_completedTasks 大小一致
	m_batchingNodeDescData.resize(m_completedTasks.size());
	for (int i = 0; i < m_batchingNodeDescData.size(); i++)
	{
		//m_batchingNodeDescData[i].relativePos = m_completedTasks[i].relativePos;
		m_batchingNodeDescData[i].size = m_completedTasks[i].size;
		m_batchingNodeDescData[i].nodeDescArrayIndex = m_completedTasks[i].nodeDescArrayIndex;
	}
	m_batchingNodeDesc.Update(m_batchingNodeDescData);
}
