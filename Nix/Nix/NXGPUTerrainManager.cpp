#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

NXGPUTerrainManager::NXGPUTerrainManager()
{
}

void NXGPUTerrainManager::Init()
{
	// �����ڴ�
	m_pTerrainBufferA = new NXBuffer("GPU Terrain Buffer A");
	m_pTerrainBufferA->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainBufferB = new NXBuffer("GPU Terrain Buffer B");
	m_pTerrainBufferB->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainFinalBuffer = new NXBuffer("GPU Terrain Final Buffer");
	m_pTerrainFinalBuffer->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainIndirectArgs = new NXBuffer("GPU Terrain Indirect Args Buffer");
	m_pTerrainIndirectArgs->Create(sizeof(int) * 3, 1);
	int a[3] = { 1, 1, 1 };
	m_pTerrainIndirectArgs->SetAll(a, 1);

	// ��ʼ��������Ŀǰ��ʼ���׶�ֻ��Ҫ������һ������
	NXGPUTerrainBlockData initData;
	initData = { 1, 1 };
	m_pTerrainBufferA->SetAll(&initData, 1);
	m_pTerrainBufferB->SetAll(nullptr, 0);
}

void NXGPUTerrainManager::UpdateCameraParams(NXCamera* pCam)
{
	m_pTerrainParamsData.m_camPos = pCam->GetTranslation();
}

void NXGPUTerrainManager::UpdateLodParams(uint32_t lod)
{
	m_pTerrainParamsData.m_currLodLevel = lod;
}
