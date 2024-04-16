#pragma once
#include "BaseDefs/DX12.h"

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice);
	~NXShaderVisibleDescriptorHeap() {}

	// ��һ�� non-shader-visible ����������cpuHandle�������� shader-Visible Heap �У�
	// Ȼ������׸��ɼ���������gpuHandle����ring buffer�е�ƫ�����������ظ�ƫ������
	const UINT Append(const size_t* cpuHandles, const size_t cpuHandlesSize);
	const D3D12_GPU_DESCRIPTOR_HANDLE Append(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

	// ��ȡDX��������ָ��
	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }

	// ֱ�ӻ�ȡ��ǰ���¿ɼ���������gpuHandle����ring buffer�е�ƫ����
	const UINT GetCurrOffset() { return m_shaderVisibleHeapOffset * m_descriptorByteSize; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT offset); // ��ȡringbuffer�� offsetλ�õ� cpuHandle��
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT offset); // ��ȡringbuffer�� offsetλ�õ� gpuHandle��

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// GPU�������ѡ��ڸ�������Ƴ�һ�� ring buffer��
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;

	// shaderVisibleHeap ��һ�� ring buffer��ÿ֡��Ҫ���� ring buffer �ϵ�ָ��ƫ��λ��
	UINT m_shaderVisibleHeapOffset = 0;
};