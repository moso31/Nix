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

	// ���ñ���buffer����Դ״̬���������������uavCounter��indirectArgs��״̬��
	void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state) override;

	ID3D12Resource* GetD3DResource() const override { return m_pBuffer.Current().Get(); }
	ID3D12Resource* GetD3DResourceUAVCounter() const override { return m_pUAVCounterBuffer.Current().Get(); }

private:
	void Set_Internal(const void* pSrcData, uint32_t arraySize, ID3D12Resource* pBuffer, ID3D12Resource* pUAVCounterBuffer);

private:
	// Buffer��Texture���ƣ���SRV��UAV�����Զ�������
	void InitSRV();
	void InitUAV();

private:
	// ��һ�»���ڴ�ص�ԭʼ��Դָ���ƫ����������SRV/UAV������λ��
	MultiFrame<ComPtr<ID3D12Resource>> m_pBuffer; 

	// ������buffer������UAV�ļ���
	MultiFrame<ComPtr<ID3D12Resource>> m_pUAVCounterBuffer;

	uint32_t m_stride;
	uint32_t m_byteSize;

	MultiFrame<D3D12_CPU_DESCRIPTOR_HANDLE> m_pSRV;
	MultiFrame<D3D12_CPU_DESCRIPTOR_HANDLE> m_pUAV;
};
