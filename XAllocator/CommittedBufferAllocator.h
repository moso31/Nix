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
		// pageBlockByteSize = �����ڴ��Ĵ�С pageFullByteSize = ����Allocator�ڴ��С. Ҫ�������2��n����
		// ����Լ�������ڴ���С��64B�����ڴ��С��2GB
		CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~CommittedBufferAllocator() {};

		// �����ڴ档������ӳ����ɵģ���Ҫ�ڻص������л�ȡ������ڴ��ַ
		void Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback);

		// �ͷ�ָ���ڴ�pMem
		// �ڴ�������ɸ�Allocator����ģ������޷��ͷ�
		void Free(uint8_t* pMem);

		// ÿ�����һ��Allocatorʱ�������������
		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

		ID3D12Device* GetDevice() { return m_pDevice; }

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorPageData;
		std::unordered_map<uint8_t*, BuddyTaskResult> m_freeMap;

		uint32_t m_pageFullByteSize; // ÿ��Allocator�����ڴ��С
	};
}
