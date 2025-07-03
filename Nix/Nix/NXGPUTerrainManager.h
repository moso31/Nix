#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"
#include "NXBuffer.h"
#include "NXTerrainCommon.h"

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

	void SetBakeTerrainTextures(const std::filesystem::path& heightMap2DArrayPath, const std::filesystem::path& minMaxZMap2DArrayPath);
	void UpdateTerrainSupportParam(int minIdX, int minIdY, int rowCount);

	Ntr<NXBuffer>& GetTerrainBufferA() { return m_pTerrainBufferA; }
	Ntr<NXBuffer>& GetTerrainBufferB() { return m_pTerrainBufferB; }
	Ntr<NXBuffer>& GetTerrainFinalBuffer() { return m_pTerrainFinalBuffer; }
	Ntr<NXBuffer>& GetTerrainIndirectArgs() { return m_pTerrainIndirectArgs; }

	Ntr<NXBuffer>& GetTerrainPatcherBuffer() { return m_pTerrainPatcherBuffer; }
	Ntr<NXBuffer>& GetTerrainDrawIndexArgs() { return m_pTerrainDrawIndexArgs; }

	Ntr<NXTexture2DArray>& GetTerrainHeightMap2DArray() { return m_pTerrainHeightMap2DArray; }
	Ntr<NXTexture2DArray>& GetTerrainMinMaxZMap2DArray() { return m_pTerrainMinMaxZMap2DArray; }

	NXConstantBuffer<NXGPUTerrainParams>& GetCBTerrainParams(uint32_t index) 
	{ 
		assert(index < TERRAIN_LOD_NUM);
		return m_pTerrainParams[index]; 
	}

	NXConstantBuffer<NXGPUTerrainSupport>& GetTerrainSupportParam() { return m_pTerrainSupport; }

	const D3D12_COMMAND_SIGNATURE_DESC& GetDrawIndexArgDesc() { return m_cmdSigDesc; }

	void Update(ID3D12GraphicsCommandList* pCmdList);

private:
	D3D12_INDIRECT_ARGUMENT_DESC m_drawIndexArgDesc[1];
	D3D12_COMMAND_SIGNATURE_DESC m_cmdSigDesc;

	uint32_t m_pTerrainBufferMaxSize = 65536;

	// 两个buffer A B之间 来回pingpong，结果输出到 final
	Ntr<NXBuffer> m_pTerrainBufferA;
	Ntr<NXBuffer> m_pTerrainBufferB;
	Ntr<NXBuffer> m_pTerrainFinalBuffer;
	Ntr<NXBuffer> m_pTerrainIndirectArgs;

	// GPU Terrain Patcher
	Ntr<NXBuffer> m_pTerrainPatcherBuffer;
	Ntr<NXBuffer> m_pTerrainDrawIndexArgs;

	static const uint32_t TERRAIN_LOD_NUM = 6; // 6个LOD等级
	NXGPUTerrainParams m_pTerrainParamsData[TERRAIN_LOD_NUM];
	NXConstantBuffer<NXGPUTerrainParams> m_pTerrainParams[TERRAIN_LOD_NUM];

	NXGPUTerrainSupport m_pTerrainSupportData;
	NXConstantBuffer<NXGPUTerrainSupport> m_pTerrainSupport;

	// 全局烘焙高度图/MinMaxZ纹理
	std::filesystem::path m_heightMap2DArrayPath;
	std::filesystem::path m_minMaxZMap2DArrayPath;
	Ntr<NXTexture2DArray> m_pTerrainHeightMap2DArray;
	Ntr<NXTexture2DArray> m_pTerrainMinMaxZMap2DArray;

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;
};
