#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"

template<typename T>
class NXConstantBufferImpl
{
public:
	struct Data
	{
		T* ptr;
		uint8_t* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

protected:
	void Create(UINT byteSize)
	{
		m_byteSize = byteSize;
		m_allByteSize = byteSize * MultiFrameSets_swapChainCount;

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocMng_CB->Alloc(byteSize, [this](const CommittedBufferAllocTaskResult& result) {
				m_data[i].cpuAddress = result.cpuAddress;
				m_data[i].gpuAddress = result.gpuAddress;
				m_data[i].ptr = reinterpret_cast<T*>(result.cpuAddress);
			});
		}
	}

public:
	void Update(const T& newData)
	{
		T* currentData = m_data.Current().ptr;
		memcpy(currentData, &newData, m_byteSize);
	}

private:
	MultiFrame<Data> m_data;
	UINT m_byteSize;
	UINT m_allByteSize;
};

template<typename T>
class NXStructuredBufferImpl
{
public:
	struct Data
	{
		T* ptr;
		uint8_t* cpuAddress;
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
	bool m_isDynamic = false;

	// 允许动态和静态两种情况；
	// 动态的话使用MultiFrame，静态的话只使用m_data[0]
	MultiFrame<Data> m_data;
	UINT m_byteSize;
};
