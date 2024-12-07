#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

template<typename T>
class NXStructuredBuffer
{
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

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUAddress()
	{
		return m_gpuAddress;
	}

	ID3D12Resource* GetD3DResourceAndOffset(uint64_t& byteOffset)
	{
		byteOffset = (uint64_t)m_memData.byteOffset;
		return NXAllocator_SB->GetD3DResource(m_memData.pAllocator);
	}

protected:
	void CreateInternal(uint32_t byteSize)
	{
		m_byteSize = byteSize;

		m_futureCB = m_promiseCB.get_future();
		NXAllocator_SB->Alloc(m_byteSize, [this](const CommittedBufferAllocTaskResult& result) {
			m_gpuAddress = result.gpuAddress;
			m_memData = result.memData;
			m_promiseCB.set_value();
			});
	}

	void FreeInternal()
	{
		NXAllocator_SB->Free(m_memData);
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	uint32_t m_arraySize;
	uint32_t m_byteSize;

	T* m_cpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
	XBuddyTaskMemData m_memData;
};
