#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

// 2024.5.12 NXBuffer<T> 模板类
// 对标 DX11 cbuffer。这里有分配内存的接口，分配出来的内存在底层交给XAllocator管理。
// 【TODO：资源释放。目前的设想是尽量交给 XAllocator处理，NXBuffer不管资源释放】
// 注意 NXBuffer 管理的是一个 T 数组，而不是单个 T。
// 这在很多情况下都是有用的，比如如果是FrameResource，就分配3份；GenerateCubeMap/PrefilterMap也会分配6份/30份内存，避免数据竞争；
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

	// 类似 dx11 updatesubresource.
	// index：仅对非FrameResource有效，指定更新哪个buffer。如果是FrameResource不需要提供index.
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
		// TODO: 光Alloc不行，还得在合适的时间Remove
		// Remove的配套机制需要XAllocator实现

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

			// 分配显存，返回对应GPU地址，和索引（在该CBAllocator下的第几页的第几偏移量）
			m_pCBAllocator->Alloc(byteSize, ResourceType_Upload, buffer.GPUVirtualAddr, buffer.pageIndex, buffer.pageOffset);

			// 分配描述符
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

	// CBuffer本体
	std::unique_ptr<NXBufferData[]> m_buffers;

	// 是否是 FrameResource，如果是FrameResource，bufferCount必须=3，并且Current()会根据交换链的帧索引判断使用哪张buffer
	bool m_isFrameResource;
	UINT m_bufferCount;
};
