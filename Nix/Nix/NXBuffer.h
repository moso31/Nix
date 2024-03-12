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

		UINT byteSize;
	};

public:
	NXBuffer() {}
	~NXBuffer() {}

	void Create(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, bool isMultiFrame)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, isMultiFrame);
	}

	void Create(UINT byteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, bool isMultiFrame)
	{
		// TODO: 光Alloc不行，还得在合适的时间Remove
		// Remove的配套机制需要XAllocator实现

		// Init
		m_pDevice = pCBAllocator->GetD3DDevice();
		m_pCBAllocator = pCBAllocator;
		m_pDescriptorAllocator = pDescriptorAllocator;
		m_isMultiFrame = isMultiFrame;

		// Create
		m_buffers = std::make_unique<NXBufferData[]>(isMultiFrame ? 3 : 1);

		for (auto& buffer : m_buffers)
		{
			m_pCBAllocator->Alloc(byteSize, ResourceType_Upload, buffer.GPUVirtualAddr, buffer.pageIndex, buffer.pageOffset);

			D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
			m_pDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buffer.GPUVirtualAddr;
			cbvDesc.SizeInBytes = (byteSize + 255) & ~255);
			m_pDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
			
			buffer.byteSize = byteSize;
		}
	}

	// like updatesubresource in dx11.
	void UpdateBuffer()
	{
		NXBufferData& currBuffer = m_isMultiFrame ? m_buffers[MultiFrameSets::swapChainIndex] : m_buffers[0];
		m_pCBAllocator->UpdateData(currBuffer.data, currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
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
