#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"

template<typename T>
class NXConstantBufferImpl
{
protected:
	void Create(UINT byteSize, ccmem::ConstantBufferAllocator* pAllocator)
	{
		m_byteSize = byteSize;
	}

public:

private:
	T m_data;

	uint8_t* m_pAddress;
	UINT m_byteSize;

	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtAddr;
};

