#pragma once
#include "BaseDefs/DX12.h"

// 2024.5.8 DX12描述符堆
// 这个描述符堆会分为两个区域，一个是稳定区域，一个是变化区域。
//		Stable Region 的描述符需要手动设置，否则不会变化；
//		Fluid Region 的描述符是一个ring Buffer，需要在每帧更新。
// m_stableCount 记录了 Stable Region 的描述符数量，也是 Stable 和 Fluid 的分界线。be like:
// 
// 0        (m_stableCount - 1)                            m_maxDescriptors
// |                 |                                             |
// +-----------------v+--------------------------------------------+
// |  Stable Region  ||                Fluid Region                |
// +-----------------++--------------------------------------------+
//
// 0 ~ (m_stableCount - 1) 是稳定区域，m_stableCount ~ (m_maxDescriptors - 1) 是变化区域。

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice, UINT stableIndex = 0);
	~NXShaderVisibleDescriptorHeap() {}

	// 将 non-shader-visible 的描述符拷贝到 shader-Visible Heap 的 Stable 区域
	// 需要手动指定具体的索引。
	const D3D12_GPU_DESCRIPTOR_HANDLE SetStableDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT index);

	// 将 non-shader-visible 的描述符拷贝到 shader-Visible Heap 的 Fluid 区域
	// 无需指定具体的索引，会根据 ring buffer 的位置自动更新。
	const D3D12_GPU_DESCRIPTOR_HANDLE SetFluidDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

	// 获取原生堆指针
	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index); // 获取ringbuffer中 offset位置的 cpuHandle。
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index); // 获取ringbuffer中 offset位置的 gpuHandle。

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// 稳定区域的描述符数量，也是稳定区域和变化区域的分界线
	// （若=0，整个描述符堆都是变化区域）
	UINT m_stableCount;

	// Fluid 区域描述符的索引，每帧在 ring buffer 上动态变化
	UINT m_fluidIndex;

	// 原始本体
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;
};