#pragma once
#include "NXResource.h"

class NXBuffer : public NXResource
{
public:
	NXBuffer(const std::string& name) : NXResource(NXResourceType::Buffer, name) {}
	virtual ~NXBuffer() {}

	void Create(uint32_t stride, uint32_t arraySize);
	uint32_t GetByteSize() const { return m_byteSize; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_pSRV; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const { return m_pUAV; }

private:
	// Buffer��Texture���ƣ���SRV��UAV�����Զ�������
	void SetSRV();
	void SetUAV();

private:
	// ��һ�»���ڴ�ص�ԭʼ��Դָ���ƫ����������SRV/UAV������λ��
	ComPtr<ID3D12Resource> m_pBuffer; 
	uint64_t m_pBufferByteOffset; 

	uint32_t m_stride;
	uint32_t m_byteSize;

	D3D12_CPU_DESCRIPTOR_HANDLE m_pSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_pUAV;
};
