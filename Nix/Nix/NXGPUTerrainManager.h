#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"
#include "NXBuffer.h"

// 2025.6.4
// GPU�������ݹ�������Ŀǰ����������е��ο�����ݣ�Ȼ�����͸�Compute Shader�����޳������Ƶȼ���
class NXGPUTerrainManager : public NXInstance<NXGPUTerrainManager>
{
public:
	NXGPUTerrainManager();
	//virtual ~NXGPUTerrainManager() {};

	void Init();

	void AddSceneTerrains(NXScene* pScene);
	void AddTerrain(Ntr<NXTerrain> pTerrain);
	void UpdateGPUTerrainNodes(NXCamera* pCamera);

	Ntr<NXBuffer> GetTerrainLodBuffer(uint32_t lodLevel);

private:
	// ��¼���еĵ��ο�
	std::vector<Ntr<NXTerrain>> m_pTerrains;

	// gpu�������ݣ��洢�������ο��AABB��Ϣ������GPU����
	std::vector<std::vector<NXGPUTerrainBlockData>> m_gpuTerrainData;

	// ����LOD�ľ������ã���λ����
	std::vector<uint32_t> m_terrainLODDistProfile; 

	std::vector<Ntr<NXBuffer>> m_pTerrainLodBuffer; // GPU�������ݻ����������ڴ��ݸ�Compute Shader
	std::vector<Ntr<NXBuffer>> m_pTerrainFinalBuffer; // GPU�������ݻ����������ڴ��ݸ�Compute Shader
};
