#pragma once
#include "DXIncludes.h"
#include "DeadList.h"

namespace ccmem
{
	struct NonVisibleDescriptorTaskResult
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	};

	class NonVisibleDescriptorAllocator : public DeadListAllocator
	{
	public:
		NonVisibleDescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize);
		virtual ~NonVisibleDescriptorAllocator() {};

		void Alloc(const std::function<void(NonVisibleDescriptorTaskResult&)>& callback);
		void Free(uint32_t freeIndex);

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		uint32_t m_descriptorIncrementSize;
	};
}
