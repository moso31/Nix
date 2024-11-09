#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>
#include <vector>

template<typename T>
class NXConstantBufferImpl
{
public:
	void WaitCreateComplete()
	{
		m_futureCB.wait();
	}

protected:
	virtual void CreateInternal(UINT byteSize) = 0;
	virtual void FreeInternal() = 0;

protected:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	std::atomic<int> m_counter = MultiFrameSets_swapChainCount;
	std::atomic<bool> m_isCreating = false;
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

	void Recreate() 
	{
		FreeInternal();
		CreateInternal(sizeof(T));
	}

	const T& Current()
	{
		return *m_cpuAddrs.Current();
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress()
	{
		WaitCreateComplete();
		return m_gpuAddrs.Current();
	}

	// ��ȡ����FrameResource��GPU��ַ����MultiFrame����ʽ���ء�
	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetFrameGPUAddresses()
	{
		WaitCreateComplete();
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

private:
	void CreateInternal(UINT byteSize) override
	{
		if (m_isCreating)
		{
			WaitCreateComplete();
		}

		m_isCreating = true;
		m_promiseCB = std::promise<void>();
		m_futureCB = m_promiseCB.get_future();
		m_counter = MultiFrameSets_swapChainCount;

		byteSize = (byteSize + 255) & ~255;
		m_byteSize = byteSize;

		// ���̴߳���CB
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Alloc(byteSize, [this, i](const CommittedBufferAllocTaskResult& result) {
				// lambda�ڿ�������һ���߳�A
				// ��ȡAlloc���ϴ���cpu��ַ��
				m_cpuAddrs[i] = reinterpret_cast<T*>(result.cpuAddress);
				m_gpuAddrs[i] = result.gpuAddress;

				if (--m_counter == 0)
				{
					m_promiseCB.set_value();
				}
				});
		}
	}

	void FreeInternal() override
	{
		if (m_isCreating)
		{
			WaitCreateComplete();
		}

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Free(reinterpret_cast<uint8_t*>(m_cpuAddrs[i]));
			m_cpuAddrs[i] = nullptr;
			m_gpuAddrs[i] = 0;
		}
	}

private:
	MultiFrame<T*> m_cpuAddrs;
	MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS> m_gpuAddrs;
};

template<typename T>
class NXConstantBuffer<std::vector<T>> : public NXConstantBufferImpl<std::vector<T>>
{
public:
	// ����չ��캯����������Ҫ�ֶ�����Recreate
	NXConstantBuffer() {}

	NXConstantBuffer(size_t arraySize) :
		m_arraySize(arraySize)
	{
		CreateInternal(sizeof(T) * m_arraySize);
	}

	~NXConstantBuffer()
	{
		FreeInternal();
	}

	void Recreate(size_t arraySize) 
	{
		m_arraySize = arraySize;
		FreeInternal();
		CreateInternal(sizeof(T) * m_arraySize);
	}

	T* Current()
	{
		return m_cpuAddrs.Current();
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUAddress()
	{
		WaitCreateComplete();
		return m_gpuAddrs.Current();
	}

	// ��ȡ����FrameResource��GPU��ַ����MultiFrame����ʽ���ء�
	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetFrameGPUAddresses()
	{
		WaitCreateComplete();
		return m_gpuAddrs;
	}

	// ������������
	void Update(const std::vector<T>& newData)
	{
		memcpy(m_cpuAddrs.Current(), newData.data(), sizeof(T) * newData.size());
	}

	// ��������MultiFrame����������
	void Set(const std::vector<T>& newData)
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			memcpy(m_cpuAddrs[i], newData.data(), sizeof(T) * newData.size());
		}
	}

private:
	void CreateInternal(UINT byteSize)
	{
		if (m_isCreating)
		{
			WaitCreateComplete();
		}

		m_isCreating = true;
		m_promiseCB = std::promise<void>();
		m_futureCB = m_promiseCB.get_future();
		m_counter = MultiFrameSets_swapChainCount;

		byteSize = (byteSize + 255) & ~255;
		m_byteSize = byteSize;

		// ���̴߳���CB
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Alloc(byteSize, [this, i](const CommittedBufferAllocTaskResult& result) {
				// lambda�ڿ�������һ���߳�A
				// ��ȡAlloc���ϴ���cpu��ַ��
				m_cpuAddrs[i] = reinterpret_cast<T*>(result.cpuAddress);
				m_gpuAddrs[i] = result.gpuAddress;

				if (--m_counter == 0)
				{
					m_promiseCB.set_value();
				}
				});
		}
	}

	void FreeInternal() override
	{
		if (m_isCreating)
		{
			WaitCreateComplete();
		}

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Free(reinterpret_cast<uint8_t*>(m_cpuAddrs[i]));
			m_cpuAddrs[i] = nullptr;
			m_gpuAddrs[i] = 0;
		}
	}

private:
	size_t m_arraySize;

	MultiFrame<T*> m_cpuAddrs;
	MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS> m_gpuAddrs;
};
