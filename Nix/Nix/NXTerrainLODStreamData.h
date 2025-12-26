#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"
#include "NXConstantBuffer.h"
#include "NXBuffer.h"
#include "NXTexture.h"

struct CBufferTerrainNodeDescription
{
	// 地形左下角XZ节点坐标（左手坐标系）
	Int2 positionWS; 

	// 节点大小，一定是2的整数幂
	uint32_t size; 

	// 节点的minmaxZ数据
	Vector2 minmaxZ; 
};

class NXTerrainLODStreamer;
class NXTerrainLODStreamData
{
public:
	static const int s_atlasHeightMapSize = 65;	// HeightMap: 65 x 65 x s_atlasLayerCount
	static const int s_atlasSplatMapSize = 65;	// SplatMap: 65 x 65 x s_atlasLayerCount
	static const int s_atlasLayerCount = 1024;	// atlas tex2darray 数组长度
	static const int s_sector2NodeIDTexSize = 256;

	NXTerrainLODStreamData() {};
	~NXTerrainLODStreamData() {};

	void Init(NXTerrainLODStreamer* pStreamer);

	const std::vector<CBufferTerrainNodeDescription>& GetNodeDescArrayData() const { return m_nodeDescArray; }
	void SetNodeDescArrayData(uint32_t index, const CBufferTerrainNodeDescription& data);
	const NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>>& GetNodeDescArray() const { return m_cbNodeDescArray; }
	void UpdateCBNodeDescArray();
	const NXConstantBuffer<std::vector<int>>& GetNodeDescUpdateIndices() const { return m_cbNodeDescUpdateIndices; }
	const uint32_t GetNodeDescUpdateIndicesNum() const { return m_nodeDescUpdateIndices.size(); }
	void ClearNodeDescUpdateIndices();

	const Ntr<NXBuffer>& GetNodeDescArrayGPUBuffer() const { return m_pTerrainNodeDescArray; }

	const Ntr<NXTexture2DArray>& GetHeightMapAtlas() const { return m_pHeightMapAtlas; }
	const Ntr<NXTexture2DArray>& GetSplatMapAtlas() const { return m_pSplatMapAtlas; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasHeightTextures() const { return m_pToAtlasHeights; }
	const std::vector<Ntr<NXTexture2D>>& GetToAtlasSplatTextures() const { return m_pToAtlasSplats; }
	void SetToAtlasHeightTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture) { m_pToAtlasHeights[index] = pTexture; }
	void SetToAtlasSplatTexture(uint32_t index, const Ntr<NXTexture2D>& pTexture) { m_pToAtlasSplats[index] = pTexture; }

private:
	// 和m_nodeDescArrayInternal完全相同，只是数据格式不同，供CPU-GPU交互
	std::vector<CBufferTerrainNodeDescription> m_nodeDescArray;
	NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>> m_cbNodeDescArray;
	// 用一个int[]记录每帧更新的nodeDesc索引，每帧重置
	std::vector<int> m_nodeDescUpdateIndices;
	NXConstantBuffer<std::vector<int>> m_cbNodeDescUpdateIndices;

	// NodeDescriptionArray(GPU)
	Ntr<NXBuffer> m_pTerrainNodeDescArray;

	// 纹理Atlas
	Ntr<NXTexture2DArray> m_pHeightMapAtlas;
	Ntr<NXTexture2DArray> m_pSplatMapAtlas;

	// 每帧待合并到Atlas的纹理列表
	std::vector<Ntr<NXTexture2D>> m_pToAtlasHeights;
	std::vector<Ntr<NXTexture2D>> m_pToAtlasSplats;
};
