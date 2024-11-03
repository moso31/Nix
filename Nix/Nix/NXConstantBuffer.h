#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>

template<typename T>
class NXConstantBufferImpl
{
public:
	void WaitCreateComplete()
	{
		m_futureCB.wait();
	}

protected:
	void CreateInternal(UINT byteSize)
	{
		byteSize = (byteSize + 255) & ~255;

		m_byteSize = byteSize;

		atomic<int> counter(MultiFrameSets_swapChainCount);
		m_futureCB = m_promiseCB.get_future();

		// 主线程创建CB
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Alloc(byteSize, [this](const CommittedBufferAllocTaskResult& result) {
				// lambda内可能是另一个线程A
				// 获取Alloc的上传堆cpu地址。
				m_cpuAddrs[i] = reinterpret_cast<T*>(result.cpuAddress);
				m_gpuAddrs[i] = result.gpuAddress;

				if (--counter == 0)
				{
					m_promiseCB.set_value();
				}

				});
		}
	}

	void FreeInternal()
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Free(m_cpuAddrs[i]);
		}
	}

protected:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	MultiFrame<T*> m_cpuAddrs;
	MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS> m_gpuAddrs;
	UINT m_byteSize;
};

template<typename T>
class NXConstantBuffer : public NXConstantBufferImpl<T>
{
public:
	NXConstantBuffer()
	{
		CreateInternal(sizeof(T));
	}

	~NXConstantBuffer()
	{
		FreeInternal();
	}

	const T& Current()
	{
		return *m_cpuAddrs.Current();
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress(size_t subOffset = 0)
	{
		WaitCreateComplete();
		return m_gpuAddrs.Current() + subOffset;
	}

	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetFrameGPUAddresses()
	{
		return m_gpuAddrs;
	}

	void Update(const T& newData)
	{
		memcpy(m_cpuAddrs.Current(), &newData, m_byteSize);
	}

	void Set(const T& newData)
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			memcpy(m_cpuAddrs[i], &newData, m_byteSize);
		}
	}
};
