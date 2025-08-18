#include "NXReadbackData.h"
#include "NXAllocatorManager.h"

void NXReadbackData::CopyDataFromGPU(uint8_t* pData)
{
    auto& dst = m_data.Current();
    std::memcpy(dst.data(), pData, static_cast<size_t>(m_byteSize));
}
