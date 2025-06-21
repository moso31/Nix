#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

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
	m_pTerrainFinalBuffer->Create(sizeof(uint32_t) * 3, m_pTerrainBufferMaxSize);

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

	// 初始化参数，目前初始化阶段只需要传入这一个地形
	NXGPUTerrainBlockData initData;
	initData = { 0, 0 };
	m_pTerrainBufferA->SetAll(&initData, 1);
	m_pTerrainBufferB->SetAll(nullptr, 0);

	// 创建一个GPU-Driven地形专用的CommandSignatureDesc
	m_cmdSigDesc = {};
	D3D12_INDIRECT_ARGUMENT_DESC args[1] = {};
	args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	m_cmdSigDesc.NumArgumentDescs = 1;
	m_cmdSigDesc.pArgumentDescs = args;
	m_cmdSigDesc.ByteStride = sizeof(int) * 5;
	m_cmdSigDesc.NodeMask = 0;

	// 各级距离
	float dist[TERRAIN_LOD_NUM] = { 10000, 2000, 600, 400, 200, 100 };
	for (int lod = 0; lod < TERRAIN_LOD_NUM; lod++)
	{
		m_pTerrainParamsData[lod].m_nodeWorldScale = (float)(64 << (5 - lod));
		m_pTerrainParamsData[lod].m_currLodLevel = lod;
		m_pTerrainParamsData[lod].m_currLodDist = dist[lod];
	}
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
