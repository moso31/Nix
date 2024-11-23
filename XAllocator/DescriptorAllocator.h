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
		DescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize, D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
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
		DescriptorAllocator(ID3D12Device* pDevice, uint32_t fullCount = 1000000, uint32_t stableCount = 10);

		virtual ~DescriptorAllocator();

		ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

		// 在上图 Stable Region 内生成一个静态描述符。
		// 是立即生效的，不需要等待。
		D3D12_GPU_DESCRIPTOR_HANDLE SetStable(uint32_t stableIndex, const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

		// 在上图 Fluid Region 内生成一个动态描述符。
		// 注意 PushFluid() 只是预处理，还需要Submit()，才能实际添加到 Fluid Region 中。
		void PushFluid(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);
		D3D12_GPU_DESCRIPTOR_HANDLE Submit();

		// 获取 Stable Region 的 CPU 和 GPU 描述符
		D3D12_CPU_DESCRIPTOR_HANDLE GetStableCPUHandle(uint32_t stableIndex) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetStableGPUHandle(uint32_t stableIndex) const;

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

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
