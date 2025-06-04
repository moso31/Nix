#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"

// 2025.6.4
// GPU地形数据管理器，目前负责接收所有地形块的数据，然后推送给Compute Shader进行剔除、绘制等计算
class NXGPUTerrainManager : public NXInstance<NXGPUTerrainManager>
{
public:
	NXGPUTerrainManager();
	//virtual ~NXGPUTerrainManager() {};

	void Init();

	void AddSceneTerrains(NXScene* pScene);
	void AddTerrain(Ntr<NXTerrain> pTerrain);
	void GetGPUTerrainNodes(NXCamera* pCamera);

private:
	// 记录所有的地形块
	std::vector<Ntr<NXTerrain>> m_pTerrains;

	// gpu地形数据，存储各个地形块的AABB信息，用于GPU计算
	std::vector<std::vector<AABB>> m_gpuTerrainData; 

	// 各级LOD的距离配置，单位：米
	std::vector<uint32_t> m_terrainLODDistProfile; 
};
