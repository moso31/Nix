#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

template<typename T>
class NXStructuredBuffer
{
	struct Data
	{
		T* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

public:
	NXStructuredBuffer(size_t arraySize)
	{
		m_arraySize = (uint32_t)arraySize;
		CreateInternal(sizeof(T) * m_arraySize);
	}

	void WaitCreateComplete()
	{
		m_futureCB.wait();
	}

	const T& GetCPUAddress()
	{
		return *m_data.cpuAddress;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUAddress()
	{
		return m_data.gpuAddress;
	}

protected:
	void CreateInternal(uint32_t byteSize)
	{
		m_byteSize = byteSize;

		m_futureCB = m_promiseCB.get_future();
		NXAllocator_SB->Alloc(m_byteSize, [this](const CommittedBufferAllocTaskResult& result) {
			m_data.cpuAddress = reinterpret_cast<T*>(result.cpuAddress);
			m_data.gpuAddress = result.gpuAddress;
			m_promiseCB.set_value();
			});
	}

	void FreeInternal()
	{
		NXAllocator_SB->Free(reinterpret_cast<uint8_t*>(m_data.cpuAddress));
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	uint32_t m_arraySize;

	Data m_data;
	uint32_t m_byteSize;
};
