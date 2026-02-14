#pragma once
#include "XAllocCommon.h"
#include "Buddy.h"

namespace ccmem
{
	struct PlacedBufferAllocTaskResult
	{
		ID3D12Resource* pResource;
		XBuddyTaskMemData memData;
	};

	class PlacedBufferAllocator : public BuddyAllocator
	{
		struct AllocatorData
		{
			ID3D12Heap* pHeap;
		};

		struct FreeData
		{
			BuddyAllocatorPage* pAllocator;
			uint32_t byteOffset;
		};;

	public:
		// pageBlockByteSize = 单个内存块的大小 pageFullByteSize = 单个Allocator内存大小. 要求必须是2的n次幂
		// 默认单个内存块大小是64B，总内存大小是2GB
		// heapFlags: Tier1 下使用的 heap flags（如 ALLOW_ONLY_BUFFERS / ALLOW_ONLY_NON_RT_DS_TEXTURES），Tier2 下自动忽略
		PlacedBufferAllocator(const std::wstring& name, ID3D12Device* pDevice, D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~PlacedBufferAllocator() {};

		// 分配内存
		void Alloc(D3D12_RESOURCE_DESC* desc, uint32_t byteSize, const std::function<void(const PlacedBufferAllocTaskResult&)>& callback);

		// 释放指定内存
		void Free(const XBuddyTaskMemData& memData);

		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorMap;

		uint32_t m_pageFullByteSize;
		D3D12_HEAP_FLAGS m_heapFlags;
	};
}
