#pragma once
#include "DXIncludes.h"
#include "Buddy.h"

namespace ccmem
{
	struct ConstantBufferAllocTaskResult
	{
		uint8_t* pMem;
	};

	class ConstantBufferAllocator : public BuddyAllocator
	{
	public:
		// blockByteSize = 单个内存块的大小 fullByteSize = 总内存大小. 要求必须是2的n次幂
		// 这里约定单个内存块大小是64B，总内存大小是2GB
		ConstantBufferAllocator(ID3D12Device* pDevice, uint32_t blockByteSize = 64, uint32_t fullByteSize = 2147483648);
		virtual ~ConstantBufferAllocator() {};

		// 分配内存
		void Alloc(uint32_t byteSize, const std::function<void(const BuddyTaskResult&)>& callback);

		// 释放指定内存pMem
		// 内存必须是由该Allocator分配的，否则无法释放
		void Free(uint8_t* pMem);

		void OnAllocatorAdded() override;

	private:
		ID3D12Device* m_pDevice;
		ID3D12Resource* m_pResource;
		uint8_t* m_pResourceData;

		uint32_t m_fullByteSize;
	};
}
