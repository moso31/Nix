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

	uint32_t GetWidth() const override { return m_byteSize / m_stride; }
	uint32_t GetHeight() const override { return 1; }
	uint32_t GetArraySize() const override { return 1; }
	uint32_t GetMipLevels() const override { return 1; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_pSRV; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return m_pUAV; }

	void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state);

private:
	// Buffer��Texture���ƣ���SRV��UAV�����Զ�������
	void SetSRV();
	void SetUAV();

private:
	// ��һ�»���ڴ�ص�ԭʼ��Դָ���ƫ����������SRV/UAV������λ��
	ComPtr<ID3D12Resource> m_pBuffer; 
	uint64_t m_pBufferByteOffset; 

	// ������buffer������UAV�ļ���
	ComPtr<ID3D12Resource> m_pUAVCounterBuffer;
	uint64_t m_pUAVCounterOffset;

	uint32_t m_stride;
	uint32_t m_byteSize;

	D3D12_CPU_DESCRIPTOR_HANDLE m_pSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_pUAV;
};
