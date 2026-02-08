#include "NXShaderVisibleDescriptorHeap.h"

NXShaderVisibleDescriptorHeap::NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice, UINT stableIndex) :
	m_pDevice(pDevice),
	m_maxDescriptors(10000),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	// 创建一个 shader-visible 的描述符堆，用于渲染前每帧提交。
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = m_maxDescriptors;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_shaderVisibleHeap));

	m_stableCount = stableIndex;
	m_fluidIndex = m_stableCount; 
}

const D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::SetStableDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT index)
{
	if (index >= m_stableCount)
	{
		assert(false);
		return { 0 };
	}

	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = { cpuHandle };
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = { m_shaderVisibleHeap->GetCPUDescriptorHandleForHeapStart().ptr + index * m_descriptorByteSize };
	m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + index * m_descriptorByteSize };
	return gpuHandle;
}

const D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::SetFluidDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = { cpuHandle };
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = { m_shaderVisibleHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_fluidIndex * m_descriptorByteSize };
	m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + m_fluidIndex * m_descriptorByteSize };
	
	if (++m_fluidIndex >= m_maxDescriptors)
	{
		m_fluidIndex = m_stableCount;
	}

	return gpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::GetCPUHandle(UINT index)
{
	return { m_shaderVisibleHeap->GetCPUDescriptorHandleForHeapStart().ptr + index * m_descriptorByteSize };
}

D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::GetGPUHandle(UINT index)
{
	return { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + index * m_descriptorByteSize };
}
