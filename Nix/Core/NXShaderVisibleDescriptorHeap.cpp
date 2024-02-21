#include "NXShaderVisibleDescriptorHeap.h"

NXShaderVisibleDescriptorHeap::NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_maxDescriptors(1000000),
	m_descriptorByteSize(pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	// ����һ�� shader-visible ���������ѣ�������Ⱦǰÿ֡�ύ��
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = m_maxDescriptors;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_shaderVisibleHeap));
}

void NXShaderVisibleDescriptorHeap::Append(const size_t* cpuHandles, const size_t cpuHandlesSize)
{
	for (size_t i = 0; i < cpuHandlesSize; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle;
		srcHandle.ptr = cpuHandles[i];

		// �����µ� ring buffer ƫ����
		UINT heapOffset = m_shaderVisibleHeapOffset * m_descriptorByteSize;
		D3D12_CPU_DESCRIPTOR_HANDLE destHandle = m_shaderVisibleHeap->GetCPUDescriptorHandleForHeapStart();
		destHandle.ptr += heapOffset;

		// ����������
		m_pDevice->CopyDescriptorsSimple(1, destHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// ����ƫ����
		m_shaderVisibleHeapOffset = (m_shaderVisibleHeapOffset + 1) % m_maxDescriptors;
	}
}

void NXShaderVisibleDescriptorHeap::Append(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	Append(&cpuHandle.ptr, 1);
}

D3D12_GPU_DESCRIPTOR_HANDLE NXShaderVisibleDescriptorHeap::GetGPUHandle(UINT gpuOffset)
{
	return { m_shaderVisibleHeap->GetGPUDescriptorHandleForHeapStart().ptr + gpuOffset * m_descriptorByteSize };
}
