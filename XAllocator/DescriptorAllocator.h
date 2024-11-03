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

	// 2024.11.3 DX12�������ѣ��ڶ��棩
	// ����������ѻ��Ϊ��������һ�����ȶ�����һ���Ǳ仯����
	//		Stable Region ����������Ҫ�ֶ����ã����򲻻�仯��
	//		Fluid Region ����������һ��ring Buffer����Ҫ��ÿ֡���¡�
	// m_stableCount ��¼�� Stable Region ��������������Ҳ�� Stable �� Fluid �ķֽ��ߡ�be like:
	// 
	// 0        (m_stableCount - 1)                            m_maxDescriptors
	// |                 |                                             |
	// +-----------------v+--------------------------------------------+
	// |  Stable Region  ||                Fluid Region                |
	// +-----------------++--------------------------------------------+
	//
	// 0 ~ (m_stableCount - 1) ���ȶ�����m_stableCount ~ (m_maxDescriptors - 1) �Ǳ仯����
	template<>
	class DescriptorAllocator<true>
	{
	public:
		DescriptorAllocator(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t fullCount = 1000000, uint32_t stableCount = 10);

		virtual ~DescriptorAllocator() {};

		ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

		// SetStable��������Ч�ģ�����Ҫ�ȴ���
		D3D12_GPU_DESCRIPTOR_HANDLE SetStable(uint32_t stableIndex, const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

		// Push����������Ч�ģ���Ҫ�ȴ�Submit
		void PushFluid(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

		// �ύ�仯������������������׸���������GPU��������㴫��������
		D3D12_GPU_DESCRIPTOR_HANDLE Submit();

	private:
		ID3D12Device* m_pDevice;
		ID3D12DescriptorHeap* m_pDescriptorHeap;

		D3D12_DESCRIPTOR_HEAP_TYPE m_eDescriptorType;
		uint32_t m_descriptorIncrementSize;
		uint32_t m_maxDescriptors;
		uint32_t m_stableCount;

		// ���ڼ�¼���ύ����������Χ
		uint32_t m_pendingStart;
		uint32_t m_pendingEnd;

		// �ȴ��ύ��������
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_submitDescriptors;
	};
}
