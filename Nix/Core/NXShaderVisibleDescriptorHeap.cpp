#include "NXShaderVisibleDescriptorHeap.h"

NXShaderVisibleDescriptorHeap::NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_maxDescriptors(1000000),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	// 创建一个 shader-visible 的描述符堆，用于渲染前每帧提交。
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = m_maxDescriptors;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_shaderVisibleHeap));
}

const D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::Append(const size_t* cpuHandles, const size_t cpuHandlesSize)
{
	UINT heapOffset = m_shaderVisibleHeapOffset * m_descriptorByteSize;
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = { m_shaderVisibleHeap->GetCPUDescriptorHandleForHeapStart().ptr + heapOffset };
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + heapOffset };

	for (size_t i = 0; i < cpuHandlesSize; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = { cpuHandles[i] };
		m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_shaderVisibleHeapOffset = (m_shaderVisibleHeapOffset + 1) % m_maxDescriptors;
		destHandle.ptr += m_descriptorByteSize;
	}

	return gpuHandle;
}

const D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::Append(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	return Append(&cpuHandle.ptr, 1);
}

D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::GetGPUHandle()
{
	return { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + m_shaderVisibleHeapOffset * m_descriptorByteSize };
}
