#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

// 2024.5.12 NXBuffer<T> 模板类
// 对标 DX11 cbuffer。这里有分配内存的接口，分配出来的内存在底层交给XAllocator管理。
// 【TODO：资源释放。目前的设想是尽量交给 XAllocator处理，NXBuffer不管资源释放】

// 允许存储多个资源，支持FrameResources
// 内存布局，以一个存储了6份数据，并且是FrameResource（交换链是三缓冲）的资源为例：
// m_buffers[0 ~ 5] = 第一帧的数据
// m_buffers[6 ~ 11] = 第二帧的数据
// m_buffers[12 ~ 17] = 第三帧的数据
template <typename T>
class NXBufferBase
{
protected:
	struct NXBufferData
	{
		T data;

		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // 记录该数据的 GPU 虚拟地址
		UINT pageIndex;
		UINT pageOffset;

		UINT byteSize;
	};

public:
	NXBufferBase() : m_isCreated(false) {}
	virtual ~NXBufferBase() {}

	// 创建一个FrameResource类型的NXBuffer。
	// FrameResource会存储交换链页数份(MultiFrameSets_swapChainCount)的数据，每一帧都会更新其中一份。
	void CreateFrameBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, true, bufferCount);
	}

	void CreateFrameBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, true, bufferCount);
	}

	// 创建一个普通的NXBuffer。
	void CreateBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	void CreateBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, false, bufferCount);
	}

	void Set(const T& data)
	{
		for (UINT i = 0; i < m_actualBufferCount; i++)
			m_buffers[i].data = data;
	}

	void Set(const T& data, UINT index)
	{
		for (UINT i = 0; i < MultiFrameSets_swapChainCount; i++)
			m_buffers[i * m_singleBufferCount + index].data = data;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS& GetGPUHandle(UINT index = 0)
	{
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index].GPUVirtualAddr : m_buffers[index].GPUVirtualAddr;
	}

	// 获取所有GPUHandle。
	// 一般在设置根参数的时候使用
	const std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& GetGPUHandleArray() { return m_buffersGPUHandles; }

	// 如果是FrameResource，获取当前帧的对应index的buffer
	// 如果不是FrameResource，获取对应index的buffer
	T& Get(UINT index = 0)
	{
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index].data : m_buffers[index].data;
	}

	// 获取指定帧的buffer
	T& Get(UINT frameIndex, UINT bufferIndex)
	{
		assert(m_isFrameResource);
		return m_buffers[frameIndex * m_singleBufferCount + bufferIndex].data;
	}

	bool IsNull()
	{
		return m_buffers.get() == nullptr;
	}

protected:
	void Create(UINT byteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, bool isFrameResource, UINT bufferCount)
	{
		// TODO: 光Alloc不行，还得在合适的时间Remove
		// Remove的配套机制需要XAllocator实现

		if (m_isCreated)
		{
			// 【TODO：启用这段逻辑。Remove没跟上之前，暂时没办法用】
			//std::wstring errMsg = L"NXBuffer create FAILED. It has been created.";
			//MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			//return;
		}

		// lazy-Init
		m_pDevice = pCBAllocator->GetD3DDevice();
		m_pCBAllocator = pCBAllocator;
		m_pDescriptorAllocator = pDescriptorAllocator;
		m_isFrameResource = isFrameResource;
		m_isCreated = true;

		// Create
		m_singleBufferCount = bufferCount;
		m_actualBufferCount = m_isFrameResource ? MultiFrameSets_swapChainCount * bufferCount : bufferCount;
		m_buffers = std::make_unique<NXBufferData[]>(m_actualBufferCount);
		m_buffersGPUHandles.reserve(m_actualBufferCount);

		for (UINT i = 0; i < m_actualBufferCount; i++)
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

			// GPUHandle 单独存一份数组
			m_buffersGPUHandles.push_back(buffer.GPUVirtualAddr);
		}
	}

protected:
	ID3D12Device*			m_pDevice;
	CommittedAllocator*		m_pCBAllocator;
	DescriptorAllocator*	m_pDescriptorAllocator;

	// 纯检测成员变量，检测是否已经调用Created方法分配内存
	// 2024.7.12 一个NXBuffer只允许Create一次
	bool m_isCreated;

	// CBuffer本体
	std::unique_ptr<NXBufferData[]> m_buffers;

	// 使用一个独立数组存一下 所有 buffer 的 GPUHandles
	// 也可以通过m_buffers中拿这个值，但调用起来不方便
	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> m_buffersGPUHandles;

	// 是否是 FrameResource，如果是FrameResource，bufferCount必须=3，并且Current()会根据交换链的帧索引判断使用哪张buffer
	bool m_isFrameResource;

	// buffer数量
	UINT m_singleBufferCount;

	// 考虑FrameResource的总buffer数量
	// 如果是FrameResource，则m_actualBufferCount=m_singleBufferCount * MultiFrameSets_swapChainCount.
	// 如果不是FrameResource，则m_actualBufferCount=m_singleBufferCount.
	UINT m_actualBufferCount;
};

template <typename T>
class NXBuffer : public NXBufferBase<T>
{
public:
	NXBuffer() {}
	virtual ~NXBuffer() {}

	// 类似 dx11 updatesubresource.
	// 如果是FrameResource，更新当前帧的对应index的buffer
	// 如果不是FrameResource，更新对应index的buffer
	void UpdateBuffer(UINT index = 0)
	{
		NXBufferData& currBuffer = m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index] : m_buffers[index];
		m_pCBAllocator->UpdateData(&currBuffer.data, currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
	}
};

// 特化 std::vector，因为Set方法总是需要深拷贝。
// 目前只有 NXCustomMaterial::m_cbData 使用此特化。
template <typename T>
class NXBuffer<std::vector<T>> : public NXBufferBase<std::vector<T>>
{
public:
	NXBuffer<std::vector<T>>() {}
	virtual ~NXBuffer<std::vector<T>>() {}

	void UpdateBuffer(UINT index = 0)
	{
		NXBufferData& currBuffer = m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index] : m_buffers[index];
		m_pCBAllocator->UpdateData(currBuffer.data.data(), currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
	}
};
