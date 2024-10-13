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

protected:
	void Create(UINT byteSize, bool isDynamic = false)
	{
		m_isDynamic = isDynamic;
		m_byteSize = byteSize;

		if (isDynamic)
		{
			for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
			{
				NXAllocMng_SB->Alloc(m_byteSize, [this](const CommittedBufferAllocTaskResult& result) {
					m_data[i].cpuAddress = result.cpuAddress;
					m_data[i].gpuAddress = result.gpuAddress;
					m_data[i].ptr = reinterpret_cast<T*>(result.cpuAddress);
					});
			}
		}
		else
		{
			NXAllocMng_SB->Alloc(m_byteSize, [this](const CommittedBufferAllocTaskResult& result) {
				m_data[0].cpuAddress = result.cpuAddress;
				m_data[0].gpuAddress = result.gpuAddress;
				m_data[0].ptr = reinterpret_cast<T*>(result.cpuAddress);
				});
		}
	}

private:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	// TODO进一步完善动态情况的处理
	bool m_isDynamic = false;

	// 允许动态和静态两种情况；
	// 动态的话使用MultiFrame，静态的话只使用m_data[0]
	MultiFrame<Data> m_data;
	UINT m_byteSize;
};
