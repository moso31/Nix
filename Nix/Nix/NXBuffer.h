#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

// 2024.5.12 NXBuffer<T> ģ����
// �Ա� DX11 cbuffer�������з����ڴ�Ľӿڣ�����������ڴ��ڵײ㽻��XAllocator����
// ��TODO����Դ�ͷš�Ŀǰ�������Ǿ������� XAllocator����NXBuffer������Դ�ͷš�
template <typename T>
class NXBuffer
{
	struct NXBufferData
	{
		T data;

		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // ��¼�����ݵ� GPU �����ַ
		UINT pageIndex;
		UINT pageOffset;

		UINT byteSize;
	};

public:
	NXBuffer() {}
	~NXBuffer() {}

	// ����һ��FrameResource���͵�NXBuffer��
	// FrameResource��洢������ҳ����(MultiFrameSets_swapChainCount)�����ݣ�ÿһ֡�����������һ�ݡ�
	void CreateFrameBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, true, MultiFrameSets_swapChainCount * bufferCount);
	}

	void CreateFrameBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, true, MultiFrameSets_swapChainCount * bufferCount);
	}

	// ����һ����ͨ��NXBuffer��
	void CreateBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	void CreateBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	// ���� dx11 updatesubresource.
	// index��ָ�������ĸ�buffer�������FrameResource
	void UpdateBuffer(UINT index = 0)
	{
		NXBufferData& currBuffer = m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index] : m_buffers[index];
		m_pCBAllocator->UpdateData(&currBuffer.data, currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUHandle(UINT index = 0)
	{
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index].GPUVirtualAddr : m_buffers[index].GPUVirtualAddr;
	}

	T& Get(UINT index = 0)
	{
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index].data : m_buffers[index].data;
	}

	void Set(T& data)
	{
		for (UINT i = 0; i < m_actualBufferCount; i++)
			m_buffers[i].data = data;
	}

	void Set(T& data, UINT index)
	{
		m_buffers[index].data = data;
	}

	bool IsNull()
	{
		return m_buffers.get() == nullptr;
	}

private:
	void Create(UINT byteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, bool isFrameResource, UINT bufferCount)
	{
		// TODO: ��Alloc���У������ں��ʵ�ʱ��Remove
		// Remove�����׻�����ҪXAllocatorʵ��

		// lazy-Init
		m_pDevice = pCBAllocator->GetD3DDevice();
		m_pCBAllocator = pCBAllocator;
		m_pDescriptorAllocator = pDescriptorAllocator;
		m_isFrameResource = isFrameResource;

		// Create
		m_singleBufferCount = bufferCount;
		m_actualBufferCount = m_isFrameResource ? MultiFrameSets_swapChainCount * bufferCount : bufferCount;
		m_buffers = std::make_unique<NXBufferData[]>(m_actualBufferCount);

		for (UINT i = 0; i < m_actualBufferCount; i++)
		{
			NXBufferData& buffer = m_buffers[i];

			// �����Դ棬���ض�ӦGPU��ַ�����������ڸ�CBAllocator�µĵڼ�ҳ�ĵڼ�ƫ������
			m_pCBAllocator->Alloc(byteSize, ResourceType_Upload, buffer.GPUVirtualAddr, buffer.pageIndex, buffer.pageOffset);

			// ����������
			D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
			m_pDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buffer.GPUVirtualAddr;
			cbvDesc.SizeInBytes = (byteSize + 255) & ~255;
			m_pDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

			buffer.byteSize = byteSize;
		}
	}

private:
	ID3D12Device*			m_pDevice;
	CommittedAllocator*		m_pCBAllocator;
	DescriptorAllocator*	m_pDescriptorAllocator;

	// CBuffer����
	std::unique_ptr<NXBufferData[]> m_buffers;

	// �Ƿ��� FrameResource�������FrameResource��bufferCount����=3������Current()����ݽ�������֡�����ж�ʹ������buffer
	bool m_isFrameResource;

	// buffer����
	UINT m_singleBufferCount;

	// ����FrameResource����buffer����
	// �����FrameResource����m_actualBufferCount=m_singleBufferCount * MultiFrameSets_swapChainCount.
	// �������FrameResource����m_actualBufferCount=m_singleBufferCount.
	UINT m_actualBufferCount;
};
