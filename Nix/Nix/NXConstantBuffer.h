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
				Data& data = m_data[i];
				data.cpuAddress = reinterpret_cast<T*>(result.cpuAddress);
				data.gpuAddress = result.gpuAddress;

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
			NXAllocator_CB->Free(m_data[i].cpuAddress);
		}
	}

protected:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	MultiFrame<Data> m_data;
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
		return *m_data.Current().cpuAddress;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress(size_t subOffset = 0)
	{
		return m_data.Current().gpuAddress + subOffset;
	}

	void Update(const T& newData)
	{
		T* currentData = m_data.Current().cpuAddress;
		memcpy(currentData, &newData, m_byteSize);
	}

	void Set(const T& newData)
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			T* currentData = m_data[i].cpuAddress;
			memcpy(currentData, &newData, m_byteSize);
		}
	}
};

template<typename T>
class NXConstantBufferArray : public NXConstantBufferImpl<T>
{
public:
	NXConstantBufferArray(uint32_t arraySize)
	{
		m_arraySize = arraySize;
		CreateInternal(sizeof(T) * m_arraySize);
	}

	~NXConstantBufferArray()
	{
		FreeInternal();
	}

	const T& Current()
	{
		return *m_data.Current().cpuAddress;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress(size_t subOffset = 0)
	{
		return m_data.Current().gpuAddress + subOffset;
	}

	void Update(const T* newDataArray)
	{
		T* currentData = m_data.Current().cpuAddress;
		memcpy(currentData, newData, m_byteSize);
	}

	void Set(const T* newDataArray)
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			T* currentData = m_data[i].cpuAddress;
			memcpy(currentData, newDataArray, m_byteSize);
		}
	}

protected:
	uint32_t m_arraySize;
};
