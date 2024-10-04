#pragma once
#include "XAllocCommon.h"
#include "Buddy.h"

namespace ccmem
{
	struct PlacedBufferAllocTaskResult
	{
		ID3D12Resource* pResource;
	};

	class PlacedBufferAllocator : public BuddyAllocator
	{
		struct AllocatorData
		{
			ID3D12Heap* pHeap;
		};

	public:
		// pageBlockByteSize = �����ڴ��Ĵ�С pageFullByteSize = ����Allocator�ڴ��С. Ҫ�������2��n����
		// ����Լ�������ڴ���С��64B�����ڴ��С��2GB
		// ��64B = һ��4x4�ֱ��ʵ�R8G8B8A8����
		PlacedBufferAllocator(ID3D12Device* pDevice, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~PlacedBufferAllocator() {};

		// �����ڴ�
		void Alloc(D3D12_RESOURCE_DESC* desc, uint32_t byteSize, const std::function<void(const PlacedBufferAllocTaskResult&)>& callback);

		// �ͷ�ָ���ڴ�
		void Free(ID3D12Resource* pFreeResource);

		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorPageData;
		std::unordered_map<ID3D12Resource*, BuddyTaskResult> m_freeMap;

		uint32_t m_pageFullByteSize;
	};
}
