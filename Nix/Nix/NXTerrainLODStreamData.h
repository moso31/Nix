#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"
#include "NXConstantBuffer.h"
#include "NXBuffer.h"

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
	void Init(NXTerrainLODStreamer* pStreamer);

	const std::vector<CBufferTerrainNodeDescription>& GetNodeDescArrayData() const { return m_nodeDescArray; }
	void SetNodeDescArrayData(size_t index, const CBufferTerrainNodeDescription& data);
	const NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>>& GetNodeDescArray() const { return m_cbNodeDescArray; }

	Ntr<NXBuffer>& GetNodeDescArrayGPUBuffer() { return m_pTerrainNodeDescArray; }

private:
	// 和m_nodeDescArrayInternal完全相同，只是数据格式不同，供CPU-GPU交互
	std::vector<CBufferTerrainNodeDescription> m_nodeDescArray;
	NXConstantBuffer<std::vector<CBufferTerrainNodeDescription>> m_cbNodeDescArray;

	// NodeDescriptionArray(GPU)
	Ntr<NXBuffer> m_pTerrainNodeDescArray;
};
