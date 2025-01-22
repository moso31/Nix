#pragma once
#include "XAllocCommon.h"
#include "Buddy.h"

namespace ccmem
{
	struct CommittedBufferAllocTaskResult
	{
		uint8_t* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
		XBuddyTaskMemData memData;
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
		CommittedBufferAllocator(ID3D12Device* pDevice, bool cpuAccessable, uint32_t pageBlockByteSize = 64, uint32_t pageFullByteSize = 2147483648);
		virtual ~CommittedBufferAllocator() {};

		// �����ڴ档������ӳ����ɵģ���Ҫ�ڻص������л�ȡ������ڴ��ַ
		void Alloc(uint32_t byteSize, const std::function<void(const CommittedBufferAllocTaskResult&)>& callback);

		// �ͷ�ָ���ڴ�pMem
		// �ڴ�������ɸ�Allocator����ģ������޷��ͷ�
		void Free(const XBuddyTaskMemData& memData);

		// ÿ�����һ��Allocatorʱ�������������
		void OnAllocatorAdded(BuddyAllocatorPage* pAllocator) override;

		// ��ȡD3D��Դ
		ID3D12Resource* GetD3DResource(BuddyAllocatorPage* pAllocator); 

		ID3D12Device* GetDevice() { return m_pDevice; }

	private:
		ID3D12Device* m_pDevice;
		std::unordered_map<BuddyAllocatorPage*, AllocatorData> m_allocatorMap;

		bool m_cpuAccessable; // �Ƿ�����CPU��
		uint32_t m_pageFullByteSize; // ÿ��Allocator�����ڴ��С
	};
}
