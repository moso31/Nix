#include "NXGPUTerrainManager.h"
#include "NXScene.h"
#include "NXCamera.h"

NXGPUTerrainManager::NXGPUTerrainManager()
{
}

void NXGPUTerrainManager::Init()
{
	return;
	// �����ڴ�
	m_pTerrainBufferA = new NXBuffer("GPU Terrain Buffer A");
	m_pTerrainBufferA->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainBufferB = new NXBuffer("GPU Terrain Buffer B");
	m_pTerrainBufferB->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	m_pTerrainFinalBuffer = new NXBuffer("GPU Terrain Final Buffer");
	m_pTerrainFinalBuffer->Create(sizeof(NXGPUTerrainBlockData), m_pTerrainBufferMaxSize);

	// ��ʼ��������Ŀǰ��ʼ���׶�ֻ��Ҫ������һ������
	NXGPUTerrainBlockData initData;
	initData = { 0, 0 };
	m_pTerrainBufferA->Set(&initData, 1);
}

void NXGPUTerrainManager::UpdateCameraParams(NXCamera* pCam)
{
	m_pTerrainParamsData.m_camPos = pCam->GetTranslation();
}

void NXGPUTerrainManager::UpdateLodParams(uint32_t lod)
{
	m_pTerrainParamsData.m_currLodLevel = lod;
}
