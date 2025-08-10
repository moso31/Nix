#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"
#include <future>
#include <atomic>
#include <vector>

class NXReadbackBufferImpl
{
public:
	void WaitCreateComplete()
	{
		m_futureCB.wait();
		m_isCreating = false;
	}

protected:
	virtual void CreateInternal(UINT byteSize) = 0;
	virtual void FreeInternal() = 0;

protected:
	std::promise<void> m_promiseCB;
	std::future<void> m_futureCB;

	std::atomic<int> m_counter = MultiFrameSets_swapChainCount;
	std::atomic<bool> m_isCreating = false;
	std::atomic<bool> m_inited = false;
	UINT m_byteSize;
};

template<typename T>
class NXReadbackBuffer : public NXReadbackBufferImpl
{
public:
	NXConstantBuffer()
	{
		CreateInternal(sizeof(T));
	}

	virtual ~NXConstantBuffer()
	{
		//FreeInternal();
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

	void Update(const T& newData)
	{
		WaitCreateComplete();
		memcpy(m_cpuAddrs.Current(), &newData, m_byteSize);
	}

	void Set(const T& newData)
	{
		WaitCreateComplete();
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

		// 主线程引导创建Buffer
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_RB->Alloc(byteSize, [this, i](const CommittedBufferAllocTaskResult& result) {
				// lambda内是另一个线程A
				// 获取Alloc的上传堆cpu地址。
				m_cpuAddrs[i] = reinterpret_cast<T*>(result.cpuAddress);
				m_memData[i] = result.memData;

				if (--m_counter == 0)
				{
					m_promiseCB.set_value();
				}
				});
		}

		m_inited = true;
	}

	void FreeInternal() override
	{
		if (m_isCreating)
		{
			WaitCreateComplete();
		}

		if (!m_inited) return;

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_RB->Free(m_memData[i]);
			m_cpuAddrs[i] = nullptr;
			m_memData[i].pAllocator = nullptr;
			m_memData[i].byteOffset = 0;
		}
	}

private:
	MultiFrame<T*> m_cpuAddrs;
	XBuddyTaskMemData m_memData[MultiFrameSets_swapChainCount];
};
