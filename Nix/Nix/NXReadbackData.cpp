#include "NXReadbackData.h"
#include "NXAllocatorManager.h"

void NXReadbackData::CopyDataFromGPU(uint8_t* pData)
{
    // 按m_byteSize的大小做全量拷贝
    std::memcpy(m_data.data(), pData, static_cast<size_t>(m_byteSize));
}

void NXReadbackData::CopyDataFromGPU(uint8_t* pSrcData, uint32_t dstOffset, uint32_t byteSize)
{
    // 仅拷贝一部分（回读纹理会用这个 需要分开拷贝footprint的每部分）
    std::memcpy(m_data.data() + dstOffset, pSrcData, static_cast<size_t>(byteSize));
}
