#pragma once
#include "NXBuffer.h"
#include "NXRWBuffer.h"
#include "NXGlobalDefinitions.h"

void NXBuffer::Create(uint32_t stride, uint32_t arraySize)
{
	m_stride = stride;
	m_byteSize = stride * arraySize;

	NXRWBuffer buffer(stride, arraySize);
	buffer.WaitCreateComplete();
	m_pBuffer = buffer.GetD3DResource();

	SetSRV();
	SetUAV();
}

void NXBuffer::SetSRV()
{
	NXAllocator_SRV->Alloc([this](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pSRV = result;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; 
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

		bool isStructured = true; // Structured or Raw，不过目前暂不启用raw
		if (isStructured)
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = m_byteSize / m_stride;
			srvDesc.Buffer.StructureByteStride = m_stride;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE; 
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = m_byteSize / sizeof(uint32_t);
			srvDesc.Buffer.StructureByteStride = 0; // Raw buffer does not have stride
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		}

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pBuffer.Get(), &srvDesc, m_pSRV);
		});
}

void NXBuffer::SetUAV()
{
	// NXAllocator_SRV 也负责创建 UAV
	NXAllocator_SRV->Alloc([this](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
		m_pUAV = result;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured buffer
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_byteSize / m_stride;
		uavDesc.Buffer.StructureByteStride = m_stride;
		uavDesc.Buffer.CounterOffsetInBytes = 0; // No counter buffer

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pBuffer.Get(), nullptr, &uavDesc, m_pUAV);
		});
}
