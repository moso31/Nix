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
	};

public:
	NXBuffer(bool isMultiFrame) : m_isMultiFrame(isMultiFrame) {}
	~NXBuffer() {}

	void Create(CommittedAllocator* pCBAllocator)
	{
		m_buffers = std::make_unique<T[]>(isMultiFrame ? 3 : 1);

		for (auto& buffer : m_buffers)
		{
			pCBAllocator->Alloc(sizeof(buffer), ResourceType_Upload, )
		}
	}

private:
	std::unique_ptr<T[]> m_buffers;
	bool m_isMultiFrame;
};
