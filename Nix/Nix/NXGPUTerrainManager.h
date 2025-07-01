#pragma once
#include "NXTerrain.h"
#include "NXInstance.h"
#include "NXBuffer.h"

struct NXGPUTerrainParams
{
	Vector3 m_camPos;
	float m_nodeWorldScale; // lod�ȼ��ĵ���node�������С
	uint32_t m_currLodLevel;
	float m_currLodDist;
};

struct NXGPUTerrainPatcherParams
{
	Matrix m_mxWorld;
	Vector3 m_pos;
	float m_pad;
	Vector2 m_uv;
	Vector2 m_terrainOrigin;
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

	void SetBakeTerrainTextures(const std::filesystem::path& heightMap2DArrayPath, const std::filesystem::path& minMaxZMap2DArrayPath);

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

	const D3D12_COMMAND_SIGNATURE_DESC& GetDrawIndexArgDesc() { return m_cmdSigDesc; }

	void Update(ID3D12GraphicsCommandList* pCmdList);

private:
	D3D12_INDIRECT_ARGUMENT_DESC m_drawIndexArgDesc[1];
	D3D12_COMMAND_SIGNATURE_DESC m_cmdSigDesc;

	uint32_t m_pTerrainBufferMaxSize = 65536;

	// ����buffer A B֮�� ����pingpong���������� final
	Ntr<NXBuffer> m_pTerrainBufferA;
	Ntr<NXBuffer> m_pTerrainBufferB;
	Ntr<NXBuffer> m_pTerrainFinalBuffer;
	Ntr<NXBuffer> m_pTerrainIndirectArgs;

	// GPU Terrain Patcher
	Ntr<NXBuffer> m_pTerrainPatcherBuffer;
	Ntr<NXBuffer> m_pTerrainDrawIndexArgs;

	static const uint32_t TERRAIN_LOD_NUM = 6; // 6��LOD�ȼ�
	NXGPUTerrainParams m_pTerrainParamsData[TERRAIN_LOD_NUM];
	NXConstantBuffer<NXGPUTerrainParams> m_pTerrainParams[TERRAIN_LOD_NUM];

	// ȫ�ֺ決�߶�ͼ/MinMaxZ����
	std::filesystem::path m_heightMap2DArrayPath;
	std::filesystem::path m_minMaxZMap2DArrayPath;
	Ntr<NXTexture2DArray> m_pTerrainHeightMap2DArray;
	Ntr<NXTexture2DArray> m_pTerrainMinMaxZMap2DArray;

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;
};
