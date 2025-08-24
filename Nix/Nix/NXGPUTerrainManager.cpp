#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "NXGlobalDefinitions.h"
#include "NXTimer.h"

NXGPUTerrainManager::NXGPUTerrainManager()
{
}

void NXGPUTerrainManager::Init()
{
	// 分配内存
	m_pTerrainBufferA = new NXBuffer("GPU Terrain Buffer A");
	m_pTerrainBufferA->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainBufferB = new NXBuffer("GPU Terrain Buffer B");
	m_pTerrainBufferB->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainFinalBuffer = new NXBuffer("GPU Terrain Final Buffer");
	m_pTerrainFinalBuffer->Create(sizeof(int) * 3, m_pTerrainBufferMaxSize);

	m_pTerrainIndirectArgs = new NXBuffer("GPU Terrain Indirect Args Buffer");
	m_pTerrainIndirectArgs->Create(sizeof(int) * 3, 1);
	int a[3] = { 1, 1, 1 };
	m_pTerrainIndirectArgs->SetAll(a, 1);

	m_pTerrainPatcherBuffer = new NXBuffer("GPU Terrain Patcher Buffer");
	m_pTerrainPatcherBuffer->Create(sizeof(NXGPUTerrainPatcherParams), m_pTerrainBufferMaxSize);

	m_pTerrainDrawIndexArgs = new NXBuffer("GPU Terrain Draw Index Indirect Args");
	m_pTerrainDrawIndexArgs->Create(sizeof(int) * 5, 1);
	int a2[5] = { 0,0,0,0,0 };
	m_pTerrainDrawIndexArgs->SetAll(a2, 1);

	// 创建一个GPU-Driven地形专用的CommandSignatureDesc
	m_drawIndexArgDesc[0] = {};
	m_drawIndexArgDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	m_cmdSigDesc = {};
	m_cmdSigDesc.NumArgumentDescs = 1;
	m_cmdSigDesc.pArgumentDescs = m_drawIndexArgDesc;
	m_cmdSigDesc.ByteStride = sizeof(int) * 5;
	m_cmdSigDesc.NodeMask = 0;

	// 各级距离
	float dist[TERRAIN_LOD_NUM] = { 6000, 3000, 1500, 800, 400, 150 };
	for (int lod = 0; lod < TERRAIN_LOD_NUM; lod++)
	{
		m_pTerrainParamsData[lod].m_nodeWorldScale = (float)(64 << (5 - lod));
		m_pTerrainParamsData[lod].m_currLodLevel = lod;
		m_pTerrainParamsData[lod].m_currLodDist = dist[lod];
	}

	UpdateTerrainSupportParam(-4, -4, 8);

	SetBakeTerrainTextures(
		"D:\\NixAssets\\terrainTest\\heightMapArray.dds", 
		"D:\\NixAssets\\terrainTest\\minMaxZMapArray.dds",
		"D:\\NixAssets\\terrainTest\\NormalArray.dds");
}

void NXGPUTerrainManager::UpdateCameraParams(NXCamera* pCam)
{
	for (auto& terrainParamData : m_pTerrainParamsData)
	{
		terrainParamData.m_camPos = pCam->GetTranslation();
	}
}

void NXGPUTerrainManager::UpdateLodParams(uint32_t lod)
{
	m_pTerrainParams[lod].Update(m_pTerrainParamsData[lod]);
}

void NXGPUTerrainManager::UpdateTerrainSupportParam(int minIdX, int minIdY, int rowCount)
{
	m_pTerrainSupportData.m_blockMinIdX = minIdX;
	m_pTerrainSupportData.m_blockMinIdY = minIdY;
	m_pTerrainSupportData.m_terrainBlockWidth = rowCount;
	m_pTerrainSupport.Set(m_pTerrainSupportData);
}

void NXGPUTerrainManager::UpdateTerrainDebugParam(float factor)
{
	m_pTerrainSupportData.m_debugParam = factor;
	m_pTerrainSupport.Set(m_pTerrainSupportData);
}

void NXGPUTerrainManager::SetBakeTerrainTextures(const std::filesystem::path& heightMap2DArrayPath, const std::filesystem::path& minMaxZMap2DArrayPath, const std::filesystem::path& NormalArrayPath)
{
	m_heightMap2DArrayPath = heightMap2DArrayPath;
	m_minMaxZMap2DArrayPath = minMaxZMap2DArrayPath;

	if (std::filesystem::exists(m_heightMap2DArrayPath))
	{
		TexMetadata metadata;
		DirectX::GetMetadataFromDDSFile(m_heightMap2DArrayPath.wstring().c_str(), DDS_FLAGS_NONE, metadata);
		m_pTerrainHeightMap2DArray = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("Terrain HeightMap 2DArray", m_heightMap2DArrayPath, DXGI_FORMAT_R16_UNORM, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
	}

	if (std::filesystem::exists(m_minMaxZMap2DArrayPath))
	{
		TexMetadata metadata;
		DirectX::GetMetadataFromDDSFile(m_minMaxZMap2DArrayPath.wstring().c_str(), DDS_FLAGS_NONE, metadata);
		m_pTerrainMinMaxZMap2DArray = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("Terrain MinMax Z Map 2DArray", m_minMaxZMap2DArrayPath, DXGI_FORMAT_R32G32_FLOAT, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
	}

	//if (std::filesystem::exists(NormalArrayPath))
	//{
	//	TexMetadata metadata;
	//	DirectX::GetMetadataFromDDSFile(NormalArrayPath.wstring().c_str(), DDS_FLAGS_NONE, metadata);
	//	m_pTerrainNormalMap2DArray = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("Terrain Normal Map 2DArray", NormalArrayPath, DXGI_FORMAT_R10G10B10A2_UNORM, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
	//}
}

void NXGPUTerrainManager::Update(ID3D12GraphicsCommandList* pCmdList)
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

	m_cbObject.Update(m_cbDataObject);

	pCmdList->SetGraphicsRootConstantBufferView(0, m_cbObject.CurrentGPUAddress());

	auto pTerrainPatchBuffer = NXGPUTerrainManager::GetInstance()->GetTerrainPatcherBuffer();
	NXShVisDescHeap->PushFluid(pTerrainPatchBuffer.IsValid() ? pTerrainPatchBuffer->GetSRV() : NXAllocator_NULL->GetNullSRV());

	auto pHeightMapTex = m_pTerrainHeightMap2DArray;
	NXShVisDescHeap->PushFluid(pHeightMapTex.IsValid() ? pHeightMapTex->GetSRV() : NXAllocator_NULL->GetNullSRV());

	auto pNormalMapTex = m_pTerrainNormalMap2DArray;
	NXShVisDescHeap->PushFluid(pNormalMapTex.IsValid() ? pNormalMapTex->GetSRV() : NXAllocator_NULL->GetNullSRV());

	auto& srvHandle = NXShVisDescHeap->Submit();
	pCmdList->SetGraphicsRootDescriptorTable(4, srvHandle); // t...
}
