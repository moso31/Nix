#pragma once
#include "BaseDefs/DX12.h"

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice);
	~NXShaderVisibleDescriptorHeap() {}

	// ��һ�飨ͨ����non-shader-visible�ģ������������� shader-Visible Heap �У�����������ring buffer�е�ƫ����
	void Append(const size_t* cpuHandles, const size_t cpuHandlesSize);
	const UINT GetOffset() const { return m_shaderVisibleHeapOffset; }

	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT gpuOffset); // ��ȡringbuffer�� offsetλ�ô��� gpuHandle��

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// GPU�������ѡ��ڸ�������Ƴ�һ�� ring buffer��
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;

	// shaderVisibleHeap ��һ�� ring buffer��ÿ֡��Ҫ���� ring buffer �ϵ�ָ��ƫ��λ��
	UINT m_shaderVisibleHeapOffset = 0;
};