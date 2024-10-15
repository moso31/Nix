#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

template<typename T>
class NXStructuredBufferImpl
{
public:
	struct Data
	{
		T* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

public:
	void WaitCreateComplete()
	{
		m_futureCB.wait();
	}

protected:
	void CreateInternal(UINT byteSize)
	{
		m_byteSize = byteSize;

		m_futureCB = m_promiseCB.get_future();
		NXAllocator_SB->Alloc(m_byteSize, [this](const CommittedBufferAllocTaskResult& result) {
			m_data.cpuAddress = result.cpuAddress;
			m_data.gpuAddress = result.gpuAddress;
			m_data.ptr = reinterpret_cast<T*>(result.cpuAddress);

			m_promiseCB.set_value();
			});
	}

	void FreeInternal()
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Free(m_data[i].cpuAddress);
		}
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	Data m_data;
	UINT m_byteSize;
};

template<typename T>
class NXStructuredBufferArray : public NXStructuredBufferImpl<UINT>
{
public:
	NXStructuredBuffer(uint32_t arraySize)
	{
		m_arraySize = arraySize;
		CreateInternal(sizeof(T) * m_arraySize);
	}

	const T& Current()
	{
		return *m_data.Current().cpuAddress;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress(size_t subOffset = 0)
	{
		return m_data.Current().gpuAddress + subOffset;
	}

protected:
	uint32_t m_arraySize;
}
