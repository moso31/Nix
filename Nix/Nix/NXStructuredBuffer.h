#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

class NXStructuredBuffer
{
public:
	using OnCreateCompleteCallback = std::function<void(NXStructuredBuffer*)>;

	NXStructuredBuffer(size_t stride, size_t arraySize, OnCreateCompleteCallback onCreateComplete = nullptr) :
		m_stride(stride),
		m_onCreateComplete(std::move(onCreateComplete))
	{
		CreateInternal(stride * arraySize);
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
			m_gpuAddress = result.gpuResource->GetGPUVirtualAddress() + result.memData.byteOffset;
			m_memData = result.memData;
			m_promiseCB.set_value();

			if (m_onCreateComplete)
			{
				m_onCreateComplete(this);
			}
			});
	}

	void FreeInternal()
	{
		NXAllocator_SB->Free(m_memData);
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	// 这俩成员存了一份 但暂时没啥用……
	uint32_t m_stride;
	uint32_t m_byteSize;

	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
	XBuddyTaskMemData m_memData;

	OnCreateCompleteCallback m_onCreateComplete;
};
