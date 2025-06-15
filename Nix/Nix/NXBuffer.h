#pragma once
#include "NXResource.h"

class NXBuffer : public NXResource
{
public:
	NXBuffer(const std::string& name) : NXResource(NXResourceType::Buffer, name) {}
	virtual ~NXBuffer() {}

	void Create(uint32_t stride, uint32_t arraySize);
	void SetCurrent(const void* pSrcData, uint32_t arraySize);
	void SetAll(const void* pSrcData, uint32_t arraySize);

	uint32_t GetByteSize() const { return m_byteSize; }

	uint32_t GetWidth() const override { return m_byteSize / m_stride; }
	uint32_t GetHeight() const override { return 1; }
	uint32_t GetArraySize() const override { return 1; }
	uint32_t GetMipLevels() const override { return 1; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_pSRV.Current(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return m_pUAV.Current(); }

	// 设置本体buffer的资源状态；但这个不会设置uavCounter、indirectArgs的状态。
	void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state) override;

	ID3D12Resource* GetD3DResource() const override { return m_pBuffer.Current().Get(); }
	ID3D12Resource* GetD3DResourceUAVCounter() const override { return m_pUAVCounterBuffer.Current().Get(); }

private:
	void Set_Internal(const void* pSrcData, uint32_t arraySize, ID3D12Resource* pBuffer, ID3D12Resource* pUAVCounterBuffer);

private:
	// Buffer和Texture相似，但SRV和UAV都是自动创建的
	void InitSRV();
	void InitUAV();

private:
	// 存一下伙伴内存池的原始资源指针和偏移量，方便SRV/UAV创建定位。
	MultiFrame<ComPtr<ID3D12Resource>> m_pBuffer; 

	// 计数器buffer，用于UAV的计数
	MultiFrame<ComPtr<ID3D12Resource>> m_pUAVCounterBuffer;

	uint32_t m_stride;
	uint32_t m_byteSize;

	MultiFrame<D3D12_CPU_DESCRIPTOR_HANDLE> m_pSRV;
	MultiFrame<D3D12_CPU_DESCRIPTOR_HANDLE> m_pUAV;
};
