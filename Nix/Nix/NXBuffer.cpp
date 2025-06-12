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

	NXRWBuffer uavCounterBuffer(sizeof(uint32_t), 1); // int: uav counter
	uavCounterBuffer.WaitCreateComplete();
	m_pUAVCounterBuffer = uavCounterBuffer.GetD3DResource();

	NXRWBuffer indirectArgsBuffer(sizeof(uint32_t), 3); // int3: x, y, z
	indirectArgsBuffer.WaitCreateComplete();
	m_pIndirectArgsBuffer = indirectArgsBuffer.GetD3DResource();

	SetSRV();
	SetUAV();
}

void NXBuffer::Set(const void* pSrcData, uint32_t arraySize)
{
	uint32_t byteSize = m_stride * arraySize;
	assert(byteSize <= m_byteSize);

	UploadTaskContext taskContext(m_name);
	if (NXUploadSystem->BuildTask(byteSize, taskContext))
	{
		auto bufDesc = m_pBuffer->GetDesc();
		uint64_t byteOffset = taskContext.pResourceOffset;
		byte* pDstData = taskContext.pResourceData + byteOffset;
		memcpy(pDstData, pSrcData, byteSize);

		taskContext.pOwner->pCmdList->CopyBufferRegion(m_pBuffer.Get(), 0, taskContext.pResource, byteOffset, byteSize);
	}

	NXUploadSystem->FinishTask(taskContext, []() {
		});

	// uav counter;
	UploadTaskContext taskCtx2(m_name + std::string("_UAVCounter"));
	if (NXUploadSystem->BuildTask(sizeof(uint32_t), taskCtx2))
	{
		auto bufDesc = m_pBuffer->GetDesc();
		uint64_t byteOffset = taskCtx2.pResourceOffset;
		byte* pDstData = taskCtx2.pResourceData + byteOffset;
		memcpy(pDstData, pSrcData, byteSize);

		taskCtx2.pOwner->pCmdList->CopyBufferRegion(m_pUAVCounterBuffer.Get(), 0, taskCtx2.pResource, byteOffset, byteSize);
	}

	NXUploadSystem->FinishTask(taskCtx2);
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
		uavDesc.Buffer.CounterOffsetInBytes = 0; // 独立的UAV counter buffer

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pBuffer.Get(), m_pUAVCounterBuffer.Get(), &uavDesc, m_pUAV);
		});
}

void NXBuffer::SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state)
{
	if (m_resourceState == state)
		return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pBuffer.Get();
	barrier.Transition.StateBefore = m_resourceState;
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &barrier);

	barrier.Transition.pResource = m_pUAVCounterBuffer.Get();
	pCommandList->ResourceBarrier(1, &barrier);

	barrier.Transition.pResource = m_pIndirectArgsBuffer.Get();
	pCommandList->ResourceBarrier(1, &barrier);

	m_resourceState = state;
}
