#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"
#include "NXBuffer.h"

struct NXGPUTerrainParams
{
	Vector3 m_camPos;
	float m_nodeWorldScale; // lod�ȼ��ĵ���node�������С
	uint32_t m_currLodLevel;
};

// 2025.6.4
// GPU�������ݹ�������Ŀǰ����������е��ο�����ݣ�Ȼ�����͸�Compute Shader�����޳������Ƶȼ���
class NXGPUTerrainManager : public NXInstance<NXGPUTerrainManager>
{
public:
	NXGPUTerrainManager();
	//virtual ~NXGPUTerrainManager() {};

	void Init();
	void UpdateCameraParams(NXCamera* pCam);
	void UpdateLodParams(uint32_t lod);

	void AddSceneTerrains(NXScene* pScene);
	void AddTerrain(Ntr<NXTerrain> pTerrain);

	Ntr<NXBuffer> GetTerrainLodBuffer(uint32_t lodLevel);
	NXConstantBuffer<NXGPUTerrainParams>& GetTerrainParams() { return m_pTerrainParams; }

private:
	// ��¼���еĵ��ο�
	//m_pTerrains[i] ��i��ʾ lod�ȼ���ֵԽСԽ��ϸ
	std::vector<Ntr<NXTerrain>> m_pTerrains;

	std::vector<std::vector<NXGPUTerrainBlockData>> m_gpuTerrainData;

	// ����һ���ּ���lod
	uint32_t m_terrainLodNum;

	std::vector<Ntr<NXBuffer>> m_pTerrainLodBuffer;
	std::vector<Ntr<NXBuffer>> m_pTerrainFinalBuffer;

	NXGPUTerrainParams m_pTerrainParamsData;
	NXConstantBuffer<NXGPUTerrainParams> m_pTerrainParams;
};
