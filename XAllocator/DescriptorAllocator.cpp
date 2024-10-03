#include "DescriptorAllocator.h"

using namespace ccmem;

NonVisibleDescriptorAllocator::NonVisibleDescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize) :
	m_pDevice(pDevice),
	DeadListAllocator(descriptorSize),
	m_descriptorIncrementSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // non-shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = descriptorSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; 

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));
	m_pDescriptorHeap->SetName(L"NonVisibleDescriptor");
}

void ccmem::NonVisibleDescriptorAllocator::Alloc(const std::function<void(NonVisibleDescriptorTaskResult&)>& callback)
{
	DeadListAllocator::Alloc([this, callback](DeadListTaskResult& result) {
		NonVisibleDescriptorTaskResult taskResult;
		taskResult.cpuHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		taskResult.cpuHandle.ptr += result * m_descriptorIncrementSize;
		callback(taskResult);
	});
}

void ccmem::NonVisibleDescriptorAllocator::Free(uint32_t freeIndex)
{
	DeadListAllocator::Free(freeIndex);
}

ccmem::ShaderVisibleDescriptorAllocator::ShaderVisibleDescriptorAllocator(ID3D12Device* pDevice, uint32_t descriptorSize) :
	m_pDevice(pDevice),
	DeadListAllocator(descriptorSize),
	m_descriptorIncrementSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shader-visible.
	desc.NodeMask = 0;
	desc.NumDescriptors = descriptorSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));
	m_pDescriptorHeap->SetName(L"ShaderVisibleDescriptor");
}

void ccmem::ShaderVisibleDescriptorAllocator::Alloc(const std::function<void(const ShaderVisibleDescriptorTaskResult&)>& callback)
{
	DeadListAllocator::Alloc([this, callback](const DeadListTaskResult& result) {
		ShaderVisibleDescriptorTaskResult taskResult;
		taskResult.cpuHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		taskResult.cpuHandle.ptr += result * m_descriptorIncrementSize;
		taskResult.gpuHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		taskResult.gpuHandle.ptr += result * m_descriptorIncrementSize;
		callback(taskResult);
	});
}

void ccmem::ShaderVisibleDescriptorAllocator::Free(uint32_t freeIndex)
{
	DeadListAllocator::Free(freeIndex);
}
