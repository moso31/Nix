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
		m_stride = stride;
		m_byteSize = stride * arraySize;

		m_data.assign(m_byteSize, 0u);
	}

	uint32_t GetStride() const { return m_stride; }
	uint32_t GetByteSize() const { return m_byteSize; }
	uint32_t GetWidth() const { return m_byteSize / m_stride; }

	void CopyDataFromGPU(uint8_t* pData);
	void CopyDataFromGPU(uint8_t* pSrcData, uint32_t dstOffset, uint32_t byteSize);
	const std::vector<uint8_t>& Get() const { return m_data; }

private:
	std::string m_name;

	uint32_t m_stride;
	uint32_t m_byteSize;

	std::vector<uint8_t> m_data;
};
