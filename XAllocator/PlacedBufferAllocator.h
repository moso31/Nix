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
		// pageBlockByteSize = �����ڴ��Ĵ�С pageFullByteSize = ����Allocator�ڴ��С. Ҫ�������2��n����
		// Ĭ�ϵ����ڴ���С��64B�����ڴ��С��2GB
		PlacedBufferAllocator(ID3D12Device* pDevice, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~PlacedBufferAllocator() {};

		// �����ڴ�
		void Alloc(D3D12_RESOURCE_DESC* desc, uint32_t byteSize, const std::function<void(const PlacedBufferAllocTaskResult&)>& callback);

		// �ͷ�ָ���ڴ�
		void Free(const XBuddyTaskMemData& memData);

		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorMap;

		uint32_t m_pageFullByteSize;
	};
}
