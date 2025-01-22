#include "DescriptorAllocator.h"

using namespace ccmem;

// <false>: non-visible descriptor

DescriptorAllocator<false>::DescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize, D3D12_DESCRIPTOR_HEAP_TYPE descriptorType) :
	m_pDevice(pDevice),
	DeadListAllocator(descriptorSize),
	m_descriptorIncrementSize(pDevice->GetDescriptorHandleIncrementSize(descriptorType))
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = descriptorSize;
	desc.Type = descriptorType;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));
	m_pDescriptorHeap->SetName(L"NonVisibleDescriptor");
}

void ccmem::DescriptorAllocator<false>::Alloc(const std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE&)>& callback)
{
	DeadListAllocator::Alloc([this, callback](const DeadListTaskResult& result) {
		D3D12_CPU_DESCRIPTOR_HANDLE taskResult;
		taskResult = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		taskResult.ptr += result.index * m_descriptorIncrementSize;
		callback(taskResult);
	});
}

void ccmem::DescriptorAllocator<false>::Free(uint32_t freeIndex)
{
	DeadListAllocator::Free(freeIndex);
}

// <true>: shader-visible descriptor

DescriptorAllocator<true>::DescriptorAllocator(ID3D12Device* pDevice, uint32_t fullCount, uint32_t stableCount) :
	m_pDevice(pDevice),
	m_descriptorIncrementSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	m_maxDescriptors(fullCount),
	m_stableCount(stableCount),
	m_pendingStart(m_stableCount),
	m_pendingEnd(m_stableCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = fullCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));
}

DescriptorAllocator<true>::~DescriptorAllocator()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap->Release();
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE ccmem::DescriptorAllocator<true>::SetStable(uint32_t stableIndex, const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	if (stableIndex >= m_stableCount)
	{
		return { 0 };
	}

	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = { cpuHandle };
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = { m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + stableIndex * m_descriptorIncrementSize };
	m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + stableIndex * m_descriptorIncrementSize };
	return gpuHandle;
}

void ccmem::DescriptorAllocator<true>::PushFluid(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	if ((uint32_t)m_submitDescriptors.size() >= m_maxDescriptors)
	{
		return;
	}

	m_submitDescriptors.push_back(cpuHandle);
	m_pendingEnd++;
}

D3D12_GPU_DESCRIPTOR_HANDLE ccmem::DescriptorAllocator<true>::Submit()
{
	if (m_submitDescriptors.empty())
	{
		return { 0 };
	}

	uint32_t descCount = (uint32_t)m_submitDescriptors.size();

	// 如果超过了最大描述符数量，就从头开始
	// 不用考虑开头是否还在使用，size非常大，一般认为已经足够安全
	if (m_pendingEnd > m_maxDescriptors)
	{
		m_pendingStart = m_stableCount;
		m_pendingEnd = m_stableCount + descCount;
	}

	for (uint32_t i = 0; i < descCount; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = m_submitDescriptors[i];
		D3D12_CPU_DESCRIPTOR_HANDLE destHandle = { m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (m_pendingStart + i) * m_descriptorIncrementSize };

		m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE result = { m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + m_pendingStart * m_descriptorIncrementSize };

	m_pendingStart = m_pendingEnd;
	m_submitDescriptors.clear();

	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE ccmem::DescriptorAllocator<true>::GetStableCPUHandle(uint32_t stableIndex) const
{
	if (stableIndex >= m_stableCount)
	{
		return { 0 };
	}

	return { m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + stableIndex * m_descriptorIncrementSize };
}

D3D12_GPU_DESCRIPTOR_HANDLE ccmem::DescriptorAllocator<true>::GetStableGPUHandle(uint32_t stableIndex) const
{
	if (stableIndex >= m_stableCount)
	{
		return { 0 };
	}

	return { m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + stableIndex * m_descriptorIncrementSize };
}

// <true>: shader-visible descriptor
