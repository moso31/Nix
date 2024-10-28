#pragma once
#include "XAllocCommon.h"
#include "DeadList.h"

namespace ccmem
{
	class NonVisibleDescriptorAllocator : public DeadListAllocator
	{
	public:
		NonVisibleDescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize);
		virtual ~NonVisibleDescriptorAllocator() {};

		void Alloc(const std::function<void(XDescriptor&)>& callback);
		void Free(uint32_t freeIndex);

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		uint32_t m_descriptorIncrementSize;
	};

	class ShaderVisibleDescriptorAllocator : public DeadListAllocator
	{
	public:
		ShaderVisibleDescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize);
		virtual ~ShaderVisibleDescriptorAllocator() {};

		void Alloc(const std::function<void(const XShaderDescriptor&)>& callback);
		void Free(uint32_t freeIndex);

		ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		uint32_t m_descriptorIncrementSize;
	};
}
