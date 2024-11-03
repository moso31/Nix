#pragma once
#include "XAllocCommon.h"
#include "DeadList.h"

namespace ccmem
{
	template<bool IsShaderVisible>
	class DescriptorAllocator;

	template<>
	class DescriptorAllocator<false> : public DeadListAllocator
	{
	public:
		DescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize);
		virtual ~DescriptorAllocator() {};

		void Alloc(const std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE&)>& callback);
		void Free(uint32_t freeIndex);

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		uint32_t m_descriptorIncrementSize;
	};

	// 2024.11.3 DX12描述符堆（第二版）
	// 这个描述符堆会分为两个区域，一个是稳定区域，一个是变化区域。
	//		Stable Region 的描述符需要手动设置，否则不会变化；
	//		Fluid Region 的描述符是一个ring Buffer，需要在每帧更新。
	// m_stableCount 记录了 Stable Region 的描述符数量，也是 Stable 和 Fluid 的分界线。be like:
	// 
	// 0        (m_stableCount - 1)                            m_maxDescriptors
	// |                 |                                             |
	// +-----------------v+--------------------------------------------+
	// |  Stable Region  ||                Fluid Region                |
	// +-----------------++--------------------------------------------+
	//
	// 0 ~ (m_stableCount - 1) 是稳定区域，m_stableCount ~ (m_maxDescriptors - 1) 是变化区域。
	template<>
	class DescriptorAllocator<true>
	{
	public:
		DescriptorAllocator(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t fullCount = 1000000, uint32_t stableCount = 10);

		virtual ~DescriptorAllocator() {};

		ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

		// SetStable是立即生效的，不需要等待。
		D3D12_GPU_DESCRIPTOR_HANDLE SetStable(uint32_t stableIndex, const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

		// Push不是立即生效的，需要等待Submit
		void PushFluid(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

		// 提交变化区域的描述符，返回首个描述符的GPU句柄（方便传描述符表）
		D3D12_GPU_DESCRIPTOR_HANDLE Submit();

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		D3D12_DESCRIPTOR_HEAP_TYPE m_eDescriptorType;
		uint32_t m_descriptorIncrementSize;
		uint32_t m_maxDescriptors;
		uint32_t m_stableCount;

		// 用于记录待提交的描述符范围
		uint32_t m_pendingStart;
		uint32_t m_pendingEnd;

		// 等待提交的描述符
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_submitDescriptors;
	};
}
