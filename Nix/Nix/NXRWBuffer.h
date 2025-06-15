#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

class NXRWBuffer
{
public:
	using OnCreateCompleteCallback = std::function<void(NXRWBuffer*)>;

	NXRWBuffer(size_t stride, size_t arraySize, OnCreateCompleteCallback onCreateComplete = nullptr) :
		m_stride(stride),
		m_onCreateComplete(std::move(onCreateComplete))
	{
		CreateInternal(stride * arraySize);
	}

	void WaitCreateComplete()
	{
		m_futureCB.wait();
	}

	MultiFrame<ID3D12Resource*>& GetD3DResource()
	{
		WaitCreateComplete();
		return m_pResource;
	}

protected:
	void CreateInternal(uint32_t byteSize)
	{
		m_byteSize = byteSize;

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		desc.Width = m_byteSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // RWBuffer UAV

		m_counter = MultiFrameSets_swapChainCount;

		m_futureCB = m_promiseCB.get_future();
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_RWB->Alloc(&desc, m_byteSize, [this, i](const PlacedBufferAllocTaskResult& result) {
				m_pResource[i] = result.pResource;
				m_memData[i] = result.memData;

				if (--m_counter == 0)
				{
					m_promiseCB.set_value();

					if (m_onCreateComplete)
					{
						m_onCreateComplete(this);
					}
				}
				});
		}
	}

	void FreeInternal()
	{
		WaitCreateComplete();
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
			NXAllocator_RWB->Free(m_memData[i]);
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;
	std::atomic<int> m_counter;

	// 这俩成员存了一份 但暂时没啥用……
	uint32_t m_stride;
	uint32_t m_byteSize;

	MultiFrame<ID3D12Resource*> m_pResource;
	MultiFrame<XBuddyTaskMemData> m_memData;

	OnCreateCompleteCallback m_onCreateComplete;
};
