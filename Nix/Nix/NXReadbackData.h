#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "NXRefCountable.h"

class NXReadbackData : public NXRefCountable
{
public:
	NXReadbackData(const std::string& name) : m_name(name) {}

	void Create(uint32_t stride, uint32_t arraySize)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_stride = stride;
		m_byteSize = stride * arraySize;

		m_data.assign(m_byteSize, 0u);
	}

	uint32_t GetStride() const { return m_stride; }
	uint32_t GetByteSize() const { return m_byteSize; }
	uint32_t GetWidth() const { return m_byteSize / m_stride; }

	// 注意这两个接口现阶段是回读线程执行的
	void CopyDataFromGPU(uint8_t* pData);
	void CopyDataFromGPU(uint8_t* pSrcData, uint32_t dstOffset, uint32_t byteSize);

	// NOTE：Get不保证线程安全！通常仅预览/快照接口使用
	const std::vector<uint8_t>& Get() const;

	// Clone线程安全
	const std::vector<uint8_t> Clone() const;

private:
	// 由于有接口需要由回读线程执行，所以上锁是有必要的
	mutable std::mutex m_mutex;
	std::string m_name;

	uint32_t m_stride;
	uint32_t m_byteSize;

	std::vector<uint8_t> m_data;
};
