#pragma once
#include "BaseDefs/DX12.h"

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice);
	~NXShaderVisibleDescriptorHeap() {}

	// 将一组（通常是non-shader-visible的）描述符拷贝到 shader-Visible Heap 中，并返回其在ring buffer中的偏移量
	const D3D12_GPU_DESCRIPTOR_HANDLE Append(const size_t* cpuHandles, const size_t cpuHandlesSize);
	const D3D12_GPU_DESCRIPTOR_HANDLE Append(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);
	const UINT GetOffset() const { return m_shaderVisibleHeapOffset; }

	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(); // 获取ringbuffer中 当前位置的 gpuHandle。

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// GPU描述符堆。在概念上设计成一个 ring buffer。
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;

	// shaderVisibleHeap 是一个 ring buffer，每帧都要更新 ring buffer 上的指针偏移位置
	UINT m_shaderVisibleHeapOffset = 0;
};