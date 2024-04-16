#pragma once
#include "BaseDefs/DX12.h"

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice);
	~NXShaderVisibleDescriptorHeap() {}

	// 将一组 non-shader-visible 的描述符（cpuHandle）拷贝到 shader-Visible Heap 中，
	// 然后更新首个可见描述符（gpuHandle）在ring buffer中的偏移量，并返回该偏移量。
	const UINT Append(const size_t* cpuHandles, const size_t cpuHandlesSize);
	const D3D12_GPU_DESCRIPTOR_HANDLE Append(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

	// 获取DX描述符堆指针
	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }

	// 直接获取当前最新可见描述符（gpuHandle）在ring buffer中的偏移量
	const UINT GetCurrOffset() { return m_shaderVisibleHeapOffset * m_descriptorByteSize; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT offset); // 获取ringbuffer中 offset位置的 cpuHandle。
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT offset); // 获取ringbuffer中 offset位置的 gpuHandle。

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// GPU描述符堆。在概念上设计成一个 ring buffer。
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;

	// shaderVisibleHeap 是一个 ring buffer，每帧都要更新 ring buffer 上的指针偏移位置
	UINT m_shaderVisibleHeapOffset = 0;
};