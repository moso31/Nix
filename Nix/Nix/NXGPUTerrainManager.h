#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"
#include "NXBuffer.h"

struct NXGPUTerrainParams
{
	Vector3 m_camPos;
	float m_nodeWorldScale; // lod等级的单个node的世界大小
	uint32_t m_currLodLevel;
};

// 2025.6.4
// GPU地形数据管理器，目前负责接收所有地形块的数据，然后推送给Compute Shader进行剔除、绘制等计算
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
	// 记录所有的地形块
	//m_pTerrains[i] 的i表示 lod等级，值越小越精细
	std::vector<Ntr<NXTerrain>> m_pTerrains;

	std::vector<std::vector<NXGPUTerrainBlockData>> m_gpuTerrainData;

	// 地形一共分几级lod
	uint32_t m_terrainLodNum;

	std::vector<Ntr<NXBuffer>> m_pTerrainLodBuffer;
	std::vector<Ntr<NXBuffer>> m_pTerrainFinalBuffer;

	NXGPUTerrainParams m_pTerrainParamsData;
	NXConstantBuffer<NXGPUTerrainParams> m_pTerrainParams;
};
