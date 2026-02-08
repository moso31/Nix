#include "NXReadbackData.h"
#include "NXAllocatorManager.h"

void NXReadbackData::CopyDataFromGPU(uint8_t* pData)
{
    // 按m_byteSize的大小做全量拷贝
    std::unique_lock<std::mutex> lock(m_mutex);
    std::memcpy(m_data.data(), pData, static_cast<size_t>(m_byteSize));
}

void NXReadbackData::CopyDataFromGPU(uint8_t* pSrcData, uint32_t dstOffset, uint32_t byteSize)
{
    // 仅拷贝一部分（回读纹理会用这个 需要分开拷贝footprint的每部分）
    std::unique_lock<std::mutex> lock(m_mutex);
    std::memcpy(m_data.data() + dstOffset, pSrcData, static_cast<size_t>(byteSize));
}

const std::vector<uint8_t>& NXReadbackData::Get() const
{
    return m_data;
}

const std::vector<uint8_t> NXReadbackData::Clone() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_data;
}
