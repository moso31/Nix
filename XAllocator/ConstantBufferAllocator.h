#pragma once
#include "DXIncludes.h"
#include "Buddy.h"

namespace ccmem
{
	struct BufferAllocTaskResult
	{
		uint8_t* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

	class CommittedBufferAllocator : public BuddyAllocator
	{
	public:
		// blockByteSize = �����ڴ��Ĵ�С fullByteSize = ���ڴ��С. Ҫ�������2��n����
		// ����Լ�������ڴ���С��64B�����ڴ��С��2GB
		CommittedBufferAllocator(ID3D12Device* pDevice, uint32_t blockByteSize = 64, uint32_t fullByteSize = 2147483648);
		virtual ~CommittedBufferAllocator() {};

		// �����ڴ�
		void Alloc(uint32_t byteSize, const std::function<void(const BufferAllocTaskResult&)>& callback);

		// �ͷ�ָ���ڴ�pMem
		// �ڴ�������ɸ�Allocator����ģ������޷��ͷ�
		void Free(uint8_t* pMem);

		void OnAllocatorAdded() override;

	private:
		ID3D12Device* m_pDevice;
		ID3D12Resource* m_pResource;
		uint8_t* m_pResourceData;

		uint32_t m_fullByteSize;
	};
}
