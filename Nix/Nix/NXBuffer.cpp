#pragma once
#include "NXBuffer.h"
#include "NXRWBuffer.h"
#include "NXGlobalDefinitions.h"
#include "NXConvertString.h"

NXBuffer::NXBuffer(const std::string& name) : 
	NXResource(NXResourceType::Buffer, name),
	m_loadingViews(0),
	m_futureLoadingViews(m_promiseLoadingViews.get_future())
{
	m_resourceState.Reset(D3D12_RESOURCE_STATE_COPY_DEST);
}

void NXBuffer::Create(uint32_t stride, uint32_t arraySize)
{
	m_stride = stride;
	m_byteSize = stride * arraySize;

	NXRWBuffer buffer(NXConvert::s2ws(m_name), stride, arraySize);
	buffer.WaitCreateComplete();
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		m_pBuffer[i] = buffer.GetD3DResource()[i];

	NXRWBuffer uavCounterBuffer(NXConvert::s2ws(m_name) + L"_counter", sizeof(uint32_t), 1);
	uavCounterBuffer.WaitCreateComplete();
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		m_pUAVCounterBuffer[i] = uavCounterBuffer.GetD3DResource()[i];
	

	// views = 1 srv + 2 uav. 
	m_loadingViews = 3; 
	InitSRV();
	InitUAV();

	// buffer 先不搞太复杂，初始化就等待加载完成
	WaitLoadingViewsFinish();
}

void NXBuffer::SetCurrent(const void* pSrcData, uint32_t arraySize)
{
	Set_Internal(pSrcData, arraySize, m_pBuffer.Current().Get(), m_pUAVCounterBuffer.Current().Get());
}

void NXBuffer::SetAll(const void* pSrcData, uint32_t arraySize)
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		Set_Internal(pSrcData, arraySize, m_pBuffer[i].Get(), m_pUAVCounterBuffer[i].Get());
}

void NXBuffer::Set_Internal(const void* pSrcData, uint32_t arraySize, ID3D12Resource* pBuffer, ID3D12Resource* pUAVCounterBuffer)
{
	uint32_t byteSize = m_stride * arraySize;
	assert(byteSize <= m_byteSize);

	if (pSrcData)
	{
		NXTransferContext taskContext(m_name);
		if (NXUploadSys->BuildTask(byteSize, taskContext))
		{
			auto bufDesc = pBuffer->GetDesc();
			uint64_t byteOffset = taskContext.pResourceOffset;
			byte* pDstData = taskContext.pResourceData + byteOffset;
			memcpy(pDstData, pSrcData, byteSize);

			taskContext.pOwner->pCmdList->CopyBufferRegion(pBuffer, 0, taskContext.pResource, byteOffset, byteSize);
		}

		NXUploadSys->FinishTask(taskContext);
	}

	// uav counter;
	uint32_t counter = arraySize;
	NXTransferContext taskCtx2(m_name + std::string("_UAVCounter"));
	if (NXUploadSys->BuildTask(sizeof(uint32_t), taskCtx2))
	{
		auto bufDesc = pUAVCounterBuffer->GetDesc();
		uint64_t byteOffset = taskCtx2.pResourceOffset;
		byte* pDstData = taskCtx2.pResourceData + byteOffset;
		memcpy(pDstData, &counter, sizeof(uint32_t));

		taskCtx2.pOwner->pCmdList->CopyBufferRegion(pUAVCounterBuffer, 0, taskCtx2.pResource, byteOffset, sizeof(uint32_t));
	}

	NXUploadSys->FinishTask(taskCtx2);
}

void NXBuffer::InitSRV()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXAllocator_SRV->Alloc([this, i](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
			m_pSRV[i] = result;

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

			NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pBuffer[i].Get(), &srvDesc, m_pSRV[i]);
			ProcessLoadingViews();
			});
	}
}

void NXBuffer::InitUAV()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		// NXAllocator_SRV 也负责创建 UAV
		NXAllocator_SRV->Alloc([this, i](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
			m_pUAV[i] = result;

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured buffer
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = m_byteSize / m_stride;
			uavDesc.Buffer.StructureByteStride = m_stride;
			uavDesc.Buffer.CounterOffsetInBytes = 0; // 独立的UAV counter buffer

			NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pBuffer[i].Get(), m_pUAVCounterBuffer[i].Get(), &uavDesc, m_pUAV[i]);
			ProcessLoadingViews();
			});

		NXAllocator_SRV->Alloc([this, i](const D3D12_CPU_DESCRIPTOR_HANDLE& result) {
			m_pUAVCounter[i] = result;

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavCounterDesc = {};
			uavCounterDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavCounterDesc.Format = DXGI_FORMAT_R32_UINT;
			uavCounterDesc.Buffer.FirstElement = 0;
			uavCounterDesc.Buffer.NumElements = 1;
			uavCounterDesc.Buffer.StructureByteStride = 0;

			NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pUAVCounterBuffer[i].Get(), nullptr, &uavCounterDesc, m_pUAVCounter[i]);
			ProcessLoadingViews();
			});
	}
}

void NXBuffer::ProcessLoadingViews()
{
	int oldVal = m_loadingViews.fetch_sub(1);

	if (oldVal == 1) // don't use m_loadingViews == 0. It will be fucked up.
	{
		m_promiseLoadingViews.set_value();
	}
}

void NXBuffer::WaitLoadingViewsFinish()
{
	m_futureLoadingViews.wait();
}

void NXBuffer::SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state)
{
	if (m_resourceState.Current() == state)
		return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pBuffer.Current().Get();
	barrier.Transition.StateBefore = m_resourceState.Current();
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &barrier);

	barrier.Transition.pResource = m_pUAVCounterBuffer.Current().Get();
	pCommandList->ResourceBarrier(1, &barrier);

	m_resourceState.Current() = state;
}
