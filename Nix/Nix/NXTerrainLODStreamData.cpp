#include "NXTerrainLODStreamData.h"
#include "NXTerrainLODStreamer.h"

void NXTerrainLODStreamData::Init(NXTerrainLODStreamer* pStreamer)
{
    // NodeDescArray_GPU 和CPU预分配相同的大小（默认1024）
	m_pTerrainNodeDescArray = new NXBuffer("Terrain NodeDescArray_GPU");
    m_pTerrainNodeDescArray->Create(sizeof(CBufferTerrainNodeDescription), pStreamer->s_nodeDescArrayInitialSize); 
}

void NXTerrainLODStreamData::SetNodeDescArrayData(size_t index, const CBufferTerrainNodeDescription& data)
{
    if (index >= m_nodeDescArray.size()) return;
    m_nodeDescArray[index] = data;
}
