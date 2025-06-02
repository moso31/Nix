#pragma once
#include "NXResource.h"

class NXBuffer : public NXResource
{
public:
	NXBuffer(const std::string& name) : NXResource(NXResourceType::Buffer, name) {}
	virtual ~NXBuffer() {}

	void Create(uint32_t stride, uint32_t arraySize);
	void Set(const void* pSrcData, uint32_t arraySize);

	uint32_t GetByteSize() const { return m_byteSize; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_pSRV; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return m_pUAV; }

	void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state);

private:
	// Buffer和Texture相似，但SRV和UAV都是自动创建的
	void SetSRV();
	void SetUAV();

private:
	// 存一下伙伴内存池的原始资源指针和偏移量，方便SRV/UAV创建定位。
	ComPtr<ID3D12Resource> m_pBuffer; 
	uint64_t m_pBufferByteOffset; 

	uint32_t m_stride;
	uint32_t m_byteSize;

	D3D12_CPU_DESCRIPTOR_HANDLE m_pSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_pUAV;
};
