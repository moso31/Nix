#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"
#include "NXConstantBuffer.h"
#include "NXBuffer.h"
#include "NXTexture.h"
#include "NXTerrainLODStreamConfigs.h"
#include "ShaderStructures.h"

// 用于延迟释放的纹理数据
struct NXTerrainTextureRemoving
{
	Ntr<NXTexture2D> pTexture;
	uint64_t fenceValue;
};

struct CBufferTerrainNodeDescription
{
	// 地形左下角XZ节点坐标（左手坐标系）
	Int2 positionWS; 

	// 节点的minmaxZ数据
	Vector2 minmaxZ; 

	// 节点大小，一定是2的整数幂
	uint32_t size; 

	uint32_t padding[3];
};

struct CBufferTerrainNodeDescUpdateInfo
{
	// 要替换的索引
	int newIndex;

	// 被替换的旧信息和大小
	// 注意如果是不需要replace，则size = 0;
	Int2 replacePosWS;
	int replaceSize; 
};

struct CBufferTerrainCullingParam
{
	// 相机位置
	Vector3 m_cameraPos;

	// 子级LOD距离（不是当前等级，是当前等级-1，为啥这么做见shader）
	float m_nextLodDist;

	// 当前consume/append的mip等级
	int m_currentMip;

	Vector3 pad;
};

struct TerrainPatchParam
{
	Int2 patchOrigin;
	int patchSize;
	int sliceIndex;
	Int2 nodeOrigin; // nodeSize = f(patchSize)
	Vector2 patchOriginPixelPos;
};

class NXCamera;
class NXTerrainLODStreamer;
class NXTerrainLODStreamData
{
public:
	NXTerrainLODStreamData() {};
	~NXTerrainLODStreamData() {};

	void Init(NXTerrainLODStreamer* pStreamer);

	const std::vector<CBufferTerrainNodeDescription>& GetNodeDescArrayData() const { return m_nodeDescArray; }
	void SetNodeDescArrayData(uint32_t index, const CBufferTerrainNodeDescription& data, const Int2& replacePosWS, int replaceSize);
	const NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>>& GetNodeDescArray() const { return m_cbNodeDescArray; }
	void UpdateCBNodeDescArray();
	const NXConstantBuffer<std::vector<CBufferTerrainNodeDescUpdateInfo>>& GetNodeDescUpdateIndices() const { return m_cbNodeDescUpdateIndices; }
	const std::vector<CBufferTerrainNodeDescUpdateInfo>& GetNodeDescUpdateIndicesData() const { return m_nodeDescUpdateIndices; }
	const uint32_t GetNodeDescUpdateIndicesNum() const { return m_nodeDescUpdateIndices.size(); }
	void ClearNodeDescUpdateIndices();

	const Ntr<NXTexture2D>& GetTerrainAlbedo2DArray() const { return m_pTerrainAlbedo2DArray; }
	const Ntr<NXTexture2D>& GetTerrainNormal2DArray() const { return m_pTerrainNormal2DArray; }
	const Ntr<NXTexture2DArray>& GetHeightMapAtlas() const { return m_pHeightMapAtlas; }
	const Ntr<NXTexture2DArray>& GetSplatMapAtlas() const { return m_pSplatMapAtlas; }
	const Ntr<NXTexture2DArray>& GetNormalMapAtlas() const { return m_pNormalMapAtlas; }
	const Ntr<NXTexture2DArray>& GetAlbedoMapAtlas() const { return m_pAlbedoMapAtlas; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasHeightTextures() const { return m_pToAtlasHeights; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasSplatTextures() const { return m_pToAtlasSplats; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasNormalTextures() const { return m_pToAtlasNormals; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasAlbedoTextures() const { return m_pToAtlasAlbedos; }
	void SetToAtlasHeightTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture);
	void SetToAtlasSplatTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture);
	void SetToAtlasNormalTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture);
	void SetToAtlasAlbedoTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture);
	const Ntr<NXTexture2D>& GetSector2NodeIDTexture() const { return m_pSector2NodeIDTexture; }

	bool NeedClearSector2NodeIDTexture() const { return m_bNeedClearSector2NodeIDTexture; }
	void MarkSector2NodeIDTextureCleared() { m_bNeedClearSector2NodeIDTexture = false; }

	const Ntr<NXBuffer>& GetPingPongNodesA() const { return m_pingpongNodesA; }
	const Ntr<NXBuffer>& GetPingPongNodesB() const { return m_pingpongNodesB; }
	const Ntr<NXBuffer>& GetPingPongNodesFinal() const { return m_pingpongNodesFinal; }
	const Ntr<NXBuffer>& GetPingPongIndirectArgs() const { return m_pingpongIndirectArgs; }

	const Ntr<NXBuffer>& GetPatcherBuffer() const { return m_patcherBuffer; }
	const Ntr<NXBuffer>& GetPatcherDrawIndexArgs() const { return m_patcherDrawArgs; }
	const Ntr<NXBuffer>& GetPatcherDrawIndexArgsZero() const { return m_patcherDrawArgsZero; }

	void UpdateCullingData(NXCamera* pCamera);
	const NXConstantBuffer<CBufferTerrainCullingParam>& GetCullingParam(uint32_t index) const { return m_cbCulling[index]; }

	void UpdateGBufferPatcherData(ID3D12GraphicsCommandList* pCmdList);

	// 每帧清理已确认GPU完成使用的纹理
	void FrameCleanup();

private:
	// 将旧纹理加入待释放队列
	void AddToRemovingQueue(const Ntr<NXTexture2D>& pTexture);

	// 和m_nodeDescArrayInternal完全相同，只是数据格式不同，供CPU-GPU交互
	std::vector<CBufferTerrainNodeDescription> m_nodeDescArray;
	NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>> m_cbNodeDescArray;
	// 用一个int[]记录每帧更新的nodeDesc索引，每帧重置
	std::vector<CBufferTerrainNodeDescUpdateInfo> m_nodeDescUpdateIndices;
	NXConstantBuffer<std::vector<CBufferTerrainNodeDescUpdateInfo>> m_cbNodeDescUpdateIndices;

	// 纹理Atlas
	Ntr<NXTexture2DArray> m_pHeightMapAtlas;
	Ntr<NXTexture2DArray> m_pSplatMapAtlas;
	Ntr<NXTexture2DArray> m_pNormalMapAtlas;
	Ntr<NXTexture2DArray> m_pAlbedoMapAtlas;

	// 地形材质纹理（存储所有terrain模块常用的材质，跟随splatMap一起使用）
	Ntr<NXTexture2D> m_pTerrainAlbedo2DArray;
	Ntr<NXTexture2D> m_pTerrainNormal2DArray;

	// 每帧待合并到Atlas的纹理列表
	std::vector<Ntr<NXTexture2D>> m_pToAtlasHeights;
	std::vector<Ntr<NXTexture2D>> m_pToAtlasSplats;
	std::vector<Ntr<NXTexture2D>> m_pToAtlasNormals;
	std::vector<Ntr<NXTexture2D>> m_pToAtlasAlbedos;

	// 待释放的纹理队列（等待GPU完成后再释放）
	std::vector<NXTerrainTextureRemoving> m_removingTextures;

	// 记录各sector的nodeID
	Ntr<NXTexture2D> m_pSector2NodeIDTexture;

	// 是否需要清空Sector2NodeID纹理（仅首帧需要）
	bool m_bNeedClearSector2NodeIDTexture = true;

	// gpu-driven ping-pong
	Ntr<NXBuffer> m_pingpongNodesA;
	Ntr<NXBuffer> m_pingpongNodesB;
	Ntr<NXBuffer> m_pingpongNodesFinal;
	Ntr<NXBuffer> m_pingpongIndirectArgs;

	// 各级ping-pong 的culling参数
	CBufferTerrainCullingParam m_cbCullingData[6];
	NXConstantBuffer<CBufferTerrainCullingParam> m_cbCulling[6];

	// gpu-driven patcher
	Ntr<NXBuffer> m_patcherBuffer;
	Ntr<NXBuffer> m_patcherDrawArgs;
	Ntr<NXBuffer> m_patcherDrawArgsZero;

	// patcher 给GBuffer用的常量缓冲区
	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;
};
