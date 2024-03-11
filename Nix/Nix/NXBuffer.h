#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

template <typename T>
class NXBuffer
{
	struct NXBufferData
	{
		T data;

		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // 记录该数据的 GPU 虚拟地址
		UINT pageIndex;
		UINT pageOffset;
	};

public:
	NXBuffer() {}
	~NXBuffer() {}

	void Create(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, bool isMultiFrame)
	{
		// Init
		m_pDevice = pCBAllocator->GetD3DDevice();
		m_pCBAllocator = pCBAllocator;
		m_pDescriptorAllocator = pDescriptorAllocator;
		m_isMultiFrame = isMultiFrame;

		// Create
		m_buffers = std::make_unique<T[]>(isMultiFrame ? 3 : 1);

		for (auto& buffer : m_buffers)
		{
			m_pCBAllocator->Alloc(sizeof(T), ResourceType_Upload, buffer.GPUVirtualAddr, buffer.pageIndex, buffer.pageOffset);

			D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
			m_pDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buffer.GPUVirtualAddr;
			cbvDesc.SizeInBytes = (sizeof(T) + 255) & ~255);
			m_pDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
		}
	}

	void UpdateBuffer()
	{
		NXBufferData& currBuffer = m_isMultiFrame ? m_buffers[MultiFrameSets::swapChainIndex] : m_buffers[0];
		m_pCBAllocator->UpdateData(currBuffer.data, sizeof(T), currBuffer.pageIndex, currBuffer.pageOffset);
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUHandle()
	{
		return m_isMultiFrame ? m_buffers[MultiFrameSets::swapChainIndex].GetGPUHandle() : m_buffers[0].GetGPUHandle();
	}

	T& Current()
	{
		return m_isMultiFrame ? m_buffers[MultiFrameSets::swapChainIndex].data : m_buffers[0].data;
	}

	T& Get(UINT index)
	{
		return m_buffers[index].data;
	}

private:
	ID3D12Device*			m_pDevice;
	CommittedAllocator*		m_pCBAllocator;
	DescriptorAllocator*	m_pDescriptorAllocator;

	std::unique_ptr<NXBufferData[]> m_buffers;
	bool m_isMultiFrame;
};
