#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

// 2024.5.12 NXBuffer<T> ģ����
// �Ա� DX11 cbuffer�������з����ڴ�Ľӿڣ�����������ڴ��ڵײ㽻��XAllocator����
// ��TODO����Դ�ͷš�Ŀǰ�������Ǿ������� XAllocator����NXBuffer������Դ�ͷš�
// ע�� NXBuffer �������һ�� T ���飬�����ǵ��� T��
// ���ںܶ�����¶������õģ����������FrameResource���ͷ���3�ݣ�GenerateCubeMap/PrefilterMapҲ�����6��/30���ڴ棬�������ݾ�����
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

	void CreateFrameBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, true, MultiFrameSets_swapChainCount);
	}

	void CreateFrameBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, true, MultiFrameSets_swapChainCount);
	}

	void CreateBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	void CreateBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	// ���� dx11 updatesubresource.
	// index�����Է�FrameResource��Ч��ָ�������ĸ�buffer�������FrameResource����Ҫ�ṩindex.
	void UpdateBuffer(UINT index = -1)
	{
		assert(m_isFrameResource || index != -1);
		NXBufferData& currBuffer = m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex] : m_buffers[index];
		m_pCBAllocator->UpdateData(&currBuffer.data, currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUHandle(UINT index = -1)
	{
		assert(m_isFrameResource || index != -1);
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex].GPUVirtualAddr : m_buffers[index].GPUVirtualAddr;
	}

	T& Current()
	{
		assert(m_isFrameResource);
		return m_buffers[MultiFrameSets::swapChainIndex].data;
	}

	T& Get(UINT index)
	{
		return m_buffers[index].data;
	}

	void Set(const T& data)
	{
		if (m_isFrameResource)
		{
			for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
				m_buffers[i].data = data;
		}
		else
		{
			m_buffers[0].data = data;
		}
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

		if (isFrameResource)
		{
			assert(bufferCount == MultiFrameSets_swapChainCount);
		}

		// Create
		m_bufferCount = bufferCount;
		m_buffers = std::make_unique<NXBufferData[]>(m_bufferCount);

		for (UINT i = 0; i < m_bufferCount; i++)
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
	UINT m_bufferCount;
};
