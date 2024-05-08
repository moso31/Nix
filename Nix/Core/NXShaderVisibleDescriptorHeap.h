#pragma once
#include "BaseDefs/DX12.h"

// 2024.5.8 DX12��������
// ����������ѻ��Ϊ��������һ�����ȶ�����һ���Ǳ仯����
//		Stable Region ����������Ҫ�ֶ����ã����򲻻�仯��
//		Fluid Region ����������һ��ring Buffer����Ҫ��ÿ֡���¡�
// m_stableCount ��¼�� Stable Region ��������������Ҳ�� Stable �� Fluid �ķֽ��ߡ�be like:
// 
// 0        (m_stableCount - 1)                            m_maxDescriptors
// |                 |                                             |
// +-----------------v+--------------------------------------------+
// |  Stable Region  ||                Fluid Region                |
// +-----------------++--------------------------------------------+
//
// 0 ~ (m_stableCount - 1) ���ȶ�����m_stableCount ~ (m_maxDescriptors - 1) �Ǳ仯����

class NXShaderVisibleDescriptorHeap
{
public:
	NXShaderVisibleDescriptorHeap(ID3D12Device* pDevice, UINT stableIndex = 0);
	~NXShaderVisibleDescriptorHeap() {}

	// �� non-shader-visible �������������� shader-Visible Heap �� Stable ����
	// ��Ҫ�ֶ�ָ�������������
	const D3D12_GPU_DESCRIPTOR_HANDLE SetStableDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT index);

	// �� non-shader-visible �������������� shader-Visible Heap �� Fluid ����
	// ����ָ������������������ ring buffer ��λ���Զ����¡�
	const D3D12_GPU_DESCRIPTOR_HANDLE SetFluidDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);

	// ��ȡԭ����ָ��
	ID3D12DescriptorHeap* GetHeap() { return m_shaderVisibleHeap.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index); // ��ȡringbuffer�� offsetλ�õ� cpuHandle��
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index); // ��ȡringbuffer�� offsetλ�õ� gpuHandle��

private:
	const UINT m_maxDescriptors;
	const UINT m_descriptorByteSize;
	ID3D12Device* m_pDevice;

	// �ȶ������������������Ҳ���ȶ�����ͱ仯����ķֽ���
	// ����=0�������������Ѷ��Ǳ仯����
	UINT m_stableCount;

	// Fluid ������������������ÿ֡�� ring buffer �϶�̬�仯
	UINT m_fluidIndex;

	// ԭʼ����
	ComPtr<ID3D12DescriptorHeap> m_shaderVisibleHeap;
};