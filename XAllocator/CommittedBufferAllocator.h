#pragma once
#include "XAllocCommon.h"
#include "Buddy.h"

namespace ccmem
{
	struct CommittedBufferAllocTaskResult
	{
		uint8_t* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

	class CommittedBufferAllocator : public BuddyAllocator
	{
		struct AllocatorData
		{
			ID3D12Resource* m_pResource;
			uint8_t* m_pResourceData;
		};

	public:
		// pageBlockByteSize = 单个内存块的大小 pageFullByteSize = 单个Allocator内存大小. 要求必须是2的n次幂
		// 这里约定单个内存块大小是64B，总内存大小是2GB
		CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~CommittedBufferAllocator() {};

		// 分配内存。结果是延迟生成的，需要在回调函数中获取分配的内存地址
		void Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback);

		// 释放指定内存pMem
		// 内存必须是由该Allocator分配的，否则无法释放
		void Free(uint8_t* pMem);

		// 每次添加一个Allocator时，调用这个函数
		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

		ID3D12Device* GetDevice() { return m_pDevice; }

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorPageData;
		std::unordered_map<uint8_t*, BuddyTaskResult> m_freeMap;

		uint32_t m_pageFullByteSize; // 每个Allocator的总内存大小
	};
}
