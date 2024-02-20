#pragma once
#include "BaseDefs/DX12.h"

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice);
	~NXShaderVisibleDescriptorHeap() {}

	// 将一组（通常是non-shader-visible的）描述符拷贝到 shader-Visible Heap 中，并返回其在ring buffer中的偏移量
	void Append(const size_t* cpuHandles, const size_t cpuHandlesSize);
	const UINT GetOffset() const { return m_shaderVisibleHeapOffset; }

	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT gpuOffset); // 获取ringbuffer中 offset位置处的 gpuHandle。

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// GPU描述符堆。在概念上设计成一个 ring buffer。
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;

	// shaderVisibleHeap 是一个 ring buffer，每帧都要更新 ring buffer 上的指针偏移位置
	UINT m_shaderVisibleHeapOffset = 0;
};