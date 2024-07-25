#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <memory>

// 2024.5.12 NXBuffer<T> ģ����
// �Ա� DX11 cbuffer�������з����ڴ�Ľӿڣ�����������ڴ��ڵײ㽻��XAllocator����
// ��TODO����Դ�ͷš�Ŀǰ�������Ǿ������� XAllocator����NXBuffer������Դ�ͷš�

// ����洢�����Դ��֧��FrameResources
// �ڴ沼�֣���һ���洢��6�����ݣ�������FrameResource���������������壩����ԴΪ����
// m_buffers[0 ~ 5] = ��һ֡������
// m_buffers[6 ~ 11] = �ڶ�֡������
// m_buffers[12 ~ 17] = ����֡������
template <typename T>
class NXBufferBase
{
protected:
	struct NXBufferData
	{
		T data;

		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // ��¼�����ݵ� GPU �����ַ
		UINT pageIndex;
		UINT pageOffset;

		UINT byteSize;
	};

public:
	NXBufferBase() : m_isCreated(false) {}
	virtual ~NXBufferBase() {}

	// ����һ��FrameResource���͵�NXBuffer��
	// FrameResource��洢������ҳ����(MultiFrameSets_swapChainCount)�����ݣ�ÿһ֡�����������һ�ݡ�
	void CreateFrameBuffers(CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(sizeof(T), pCBAllocator, pDescriptorAllocator, true, bufferCount);
	}

	void CreateFrameBuffers(UINT customByteSize, CommittedAllocator* pCBAllocator, DescriptorAllocator* pDescriptorAllocator, UINT bufferCount = 1)
	{
		Create(customByteSize, pCBAllocator, pDescriptorAllocator, true, bufferCount);
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

	// ��ȡ����GPUHandle��
	// һ�������ø�������ʱ��ʹ��
	const std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& GetGPUHandleArray() { return m_buffersGPUHandles; }

	// �����FrameResource����ȡ��ǰ֡�Ķ�Ӧindex��buffer
	// �������FrameResource����ȡ��Ӧindex��buffer
	T& Get(UINT index = 0)
	{
		return m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index].data : m_buffers[index].data;
	}

	// ��ȡָ��֡��buffer
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
		// TODO: ��Alloc���У������ں��ʵ�ʱ��Remove
		// Remove�����׻�����ҪXAllocatorʵ��

		if (m_isCreated)
		{
			// ��TODO����������߼���Removeû����֮ǰ����ʱû�취�á�
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

			// GPUHandle ������һ������
			m_buffersGPUHandles.push_back(buffer.GPUVirtualAddr);
		}
	}

protected:
	ID3D12Device*			m_pDevice;
	CommittedAllocator*		m_pCBAllocator;
	DescriptorAllocator*	m_pDescriptorAllocator;

	// ������Ա����������Ƿ��Ѿ�����Created���������ڴ�
	// 2024.7.12 һ��NXBufferֻ����Createһ��
	bool m_isCreated;

	// CBuffer����
	std::unique_ptr<NXBufferData[]> m_buffers;

	// ʹ��һ�����������һ�� ���� buffer �� GPUHandles
	// Ҳ����ͨ��m_buffers�������ֵ������������������
	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> m_buffersGPUHandles;

	// �Ƿ��� FrameResource�������FrameResource��bufferCount����=3������Current()����ݽ�������֡�����ж�ʹ������buffer
	bool m_isFrameResource;

	// buffer����
	UINT m_singleBufferCount;

	// ����FrameResource����buffer����
	// �����FrameResource����m_actualBufferCount=m_singleBufferCount * MultiFrameSets_swapChainCount.
	// �������FrameResource����m_actualBufferCount=m_singleBufferCount.
	UINT m_actualBufferCount;
};

template <typename T>
class NXBuffer : public NXBufferBase<T>
{
public:
	NXBuffer() {}
	virtual ~NXBuffer() {}

	// ���� dx11 updatesubresource.
	// �����FrameResource�����µ�ǰ֡�Ķ�Ӧindex��buffer
	// �������FrameResource�����¶�Ӧindex��buffer
	void UpdateBuffer(UINT index = 0)
	{
		NXBufferData& currBuffer = m_isFrameResource ? m_buffers[MultiFrameSets::swapChainIndex * m_singleBufferCount + index] : m_buffers[index];
		m_pCBAllocator->UpdateData(&currBuffer.data, currBuffer.byteSize, currBuffer.pageIndex, currBuffer.pageOffset);
	}
};

// �ػ� std::vector����ΪSet����������Ҫ�����
// Ŀǰֻ�� NXCustomMaterial::m_cbData ʹ�ô��ػ���
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
