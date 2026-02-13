#include "NXTerrainLODStreamData.h"
#include "NXTerrainLODStreamer.h"
#include "NXResourceManager.h"
#include "NXCamera.h"
#include "NXGlobalDefinitions.h"
#include "NXTimer.h"
#include <algorithm>

void NXTerrainLODStreamData::Init(NXTerrainLODStreamer* pStreamer)
{
	// NodeDescArray_CPU
    m_cbNodeDescArray.Recreate(g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_nodeDescArray.resize(g_terrainStreamConfig.NodeDescArrayInitialSize);
	m_cbNodeDescUpdateIndices.Recreate(g_terrainStreamConfig.MaxComputeLimit);

	// 纹理Atlas
	m_pHeightMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_HeightMapAtlas", DXGI_FORMAT_R16_UNORM, g_terrainStreamConfig.AtlasHeightMapSize, g_terrainStreamConfig.AtlasHeightMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pSplatMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_SplatMapAtlas", DXGI_FORMAT_R8_UNORM, g_terrainStreamConfig.AtlasSplatMapSize, g_terrainStreamConfig.AtlasSplatMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pNormalMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_NormalMapAtlas", DXGI_FORMAT_R8G8B8A8_UNORM, g_terrainStreamConfig.AtlasNormalMapSize, g_terrainStreamConfig.AtlasNormalMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pAlbedoMapAtlas = NXManager_Tex->CreateTexture2DArray("TerrainStreaming_AlbedoMapAtlas", DXGI_FORMAT_R8G8B8A8_UNORM, g_terrainStreamConfig.AtlasAlbedoMapSize, g_terrainStreamConfig.AtlasAlbedoMapSize, g_terrainStreamConfig.AtlasLayerCount, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 地形材质纹理（存储所有terrain模块常用的材质，跟随splatMap一起使用）
	m_pTerrainAlbedo2DArray = NXManager_Tex->CreateTexture2DArray("Terrain Albedo Array", L"D:\\NixAssets\\Terrain\\terrainAlbedo2DArray.dds");
	m_pTerrainNormal2DArray = NXManager_Tex->CreateTexture2DArray("Terrain Normal Array", L"D:\\NixAssets\\Terrain\\terrainNormal2DArray.dds");

	// 每帧待合并到Atlas的纹理列表
	m_pToAtlasHeights.resize(g_terrainStreamConfig.MaxComputeLimit);
	m_pToAtlasSplats.resize(g_terrainStreamConfig.MaxComputeLimit);
	m_pToAtlasNormals.resize(g_terrainStreamConfig.MaxComputeLimit);
	m_pToAtlasAlbedos.resize(g_terrainStreamConfig.MaxComputeLimit);

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
		int childIdx = std::max(val - 1, 0);
		m_cbCullingData[i].m_nextLodDist = g_terrainStreamConfig.DistRanges[childIdx];
		m_cbCullingData[i].m_currentMip = val;
	}

	// gpu-driven patcher
	m_patcherBuffer = new NXBuffer("GPU Terrain Patcher Buffer"); 
	m_patcherBuffer->Create(sizeof(TerrainPatchParam), 32768);

	int a2[5] = { 0,0,0,0,0 };
	m_patcherDrawArgs = new NXBuffer("GPU Terrain Draw Index Indirect Args");
	m_patcherDrawArgs->Create(sizeof(int) * 5, 1);
	m_patcherDrawArgs->SetAll(a2, 1);

	int a3[5] = { 384,0,0,0,0 }; // 384=FarCry5米字形索引数，详见SubMeshTerrain的实现
	m_patcherDrawArgsZero = new NXBuffer("GPU Terrain Draw Index Args Zero");
	m_patcherDrawArgsZero->Create(sizeof(int) * 5, 1);
	m_patcherDrawArgsZero->SetAll(a3, 1);
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

void NXTerrainLODStreamData::UpdateGBufferPatcherData(ID3D12GraphicsCommandList* pCmdList)
{
	auto* pCamera = NXResourceManager::GetInstance()->GetCameraManager()->GetCamera("Main Camera");
	auto& mxView = pCamera->GetViewMatrix();
	auto& mxWorld = Matrix::Identity();
	auto& mxWorldView = mxWorld * mxView;

	m_cbDataObject.world = mxWorld.Transpose();
	m_cbDataObject.worldInverseTranspose = mxWorld.Invert(); // it actually = m_worldMatrix.Invert().Transpose().Transpose();
	m_cbDataObject.worldView = mxWorldView.Transpose();
	m_cbDataObject.worldViewInverseTranspose = (mxWorldView).Invert();
	m_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();
	m_cbDataObject.globalData.frameIndex = (uint32_t)NXGlobalApp::s_frameIndex.load();
	m_cbObject.Update(m_cbDataObject);
	pCmdList->SetGraphicsRootConstantBufferView(0, m_cbObject.CurrentGPUAddress());

	NXShVisDescHeap->PushFluid(m_patcherBuffer.IsValid() ? m_patcherBuffer->GetSRV() : NXAllocator_NULL->GetNullSRV()); 
	NXShVisDescHeap->PushFluid(m_pHeightMapAtlas.IsValid() ? m_pHeightMapAtlas->GetSRV() : NXAllocator_NULL->GetNullSRV());
	NXShVisDescHeap->PushFluid(m_pSplatMapAtlas.IsValid() ? m_pSplatMapAtlas->GetSRV() : NXAllocator_NULL->GetNullSRV());
	NXShVisDescHeap->PushFluid(m_pNormalMapAtlas.IsValid() ? m_pNormalMapAtlas->GetSRV() : NXAllocator_NULL->GetNullSRV());
	NXShVisDescHeap->PushFluid(m_pAlbedoMapAtlas.IsValid() ? m_pAlbedoMapAtlas->GetSRV() : NXAllocator_NULL->GetNullSRV());
	NXShVisDescHeap->PushFluid(m_pTerrainAlbedo2DArray.IsValid() ? m_pTerrainAlbedo2DArray->GetSRV() : NXAllocator_NULL->GetNullSRV());
	NXShVisDescHeap->PushFluid(m_pTerrainNormal2DArray.IsValid() ? m_pTerrainNormal2DArray->GetSRV() : NXAllocator_NULL->GetNullSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	pCmdList->SetGraphicsRootDescriptorTable(4, srvHandle); // GPU-Driven Terrain 使用4号根参数 具体见NXCustomMaterial::CompileShader()
}

void NXTerrainLODStreamData::SetToAtlasHeightTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture)
{
	AddToRemovingQueue(m_pToAtlasHeights[index]);
	m_pToAtlasHeights[index] = pTexture;
}

void NXTerrainLODStreamData::SetToAtlasSplatTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture)
{
	AddToRemovingQueue(m_pToAtlasSplats[index]);
	m_pToAtlasSplats[index] = pTexture;
}

void NXTerrainLODStreamData::SetToAtlasNormalTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture)
{
	AddToRemovingQueue(m_pToAtlasNormals[index]);
	m_pToAtlasNormals[index] = pTexture;
}

void NXTerrainLODStreamData::SetToAtlasAlbedoTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture)
{
	AddToRemovingQueue(m_pToAtlasAlbedos[index]);
	m_pToAtlasAlbedos[index] = pTexture;
}

void NXTerrainLODStreamData::AddToRemovingQueue(const Ntr<NXTexture2D>& pTexture)
{
	if (pTexture.IsNull())
		return;

	NXTerrainTextureRemoving removing;
	removing.pTexture = pTexture;
	removing.fenceValue = NXGlobalDX::s_globalfenceValue;
	m_removingTextures.push_back(removing);
}

void NXTerrainLODStreamData::FrameCleanup()
{
	UINT64 currentGPUFenceValue = NXGlobalDX::s_globalfence->GetCompletedValue();
	std::erase_if(m_removingTextures, [currentGPUFenceValue](const NXTerrainTextureRemoving& removing)
		{
			return currentGPUFenceValue > removing.fenceValue;
		});
}
