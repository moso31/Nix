#include "DX12.h"

void NX12Util::CreateCommands(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandQueue* oCmdQueue, ID3D12CommandAllocator* oCmdAllocator, ID3D12GraphicsCommandList* oCmdList, bool disableGPUTimeOut)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Flags = disableGPUTimeOut ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hr;
	hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&oCmdQueue));
	hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&oCmdAllocator));
	hr = pDevice->CreateCommandList(0, type, oCmdAllocator, nullptr, IID_PPV_ARGS(&oCmdList));
}

ID3D12CommandQueue* NX12Util::CreateCommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandQueue* pCmdQueue;
	HRESULT hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pCmdQueue));
	return pCmdQueue;
}

ID3D12CommandList* NX12Util::CreateCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* oCmdAllocator, D3D12_COMMAND_LIST_TYPE type, UINT nodeMask, ID3D12PipelineState* InitState)
{
	ID3D12CommandList* pCmdList;
	HRESULT hr = pDevice->CreateCommandList(nodeMask, type, oCmdAllocator, InitState, IID_PPV_ARGS(&pCmdList));
	return pCmdList;
}

ID3D12Resource* NX12Util::CreateBuffer(ID3D12Device* pDevice, const std::string& name, UINT sizeOfByte, D3D12_HEAP_TYPE heapType)
{
	ID3D12Resource* pResource = nullptr;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = ByteAlign256(sizeOfByte);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = pDevice->CreateCommittedResource(
		&CreateHeapProperties(heapType),
		D3D12_HEAP_FLAG_NONE,
		&desc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初始的资源状态为READ（允许CPU写入数据）
		nullptr,
		IID_PPV_ARGS(&pResource)
	);

	std::wstring wName(name.begin(), name.end());
	pResource->SetName(wName.c_str());

	if (FAILED(hr))
	{
		pResource->Release();
		delete pResource;
		return nullptr;
	}

	return pResource;
}

D3D12_RESOURCE_DESC NX12Util::CreateResourceDesc_DepthStencil(UINT width, UINT height, DXGI_FORMAT fmt)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.Format = fmt;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	return desc;
}

D3D12_HEAP_PROPERTIES NX12Util::CreateHeapProperties(D3D12_HEAP_TYPE heapType)
{
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = heapType;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;
	return hp;
}

D3D12_CLEAR_VALUE NX12Util::CreateClearValue(float depth, UINT8 stencil, DXGI_FORMAT fmt)
{
	D3D12_CLEAR_VALUE cv;
	cv.Format = fmt;
	cv.DepthStencil.Depth = depth;
	cv.DepthStencil.Stencil = stencil;
	return cv;
}

void NX12Util::SetResourceBarrier(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = pResource;
	barrier.Transition.StateBefore = from;
	barrier.Transition.StateAfter = to;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &barrier);
}

void NX12Util::CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT layoutSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts)
{
	for (UINT idx = 0; idx < layoutSize; ++idx)
	{
		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.SubresourceIndex = idx;
		dst.PlacedFootprint = {};
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.pResource = pTexture;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.SubresourceIndex = 0;
		src.PlacedFootprint = pLayouts[idx];
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.pResource = pTextureUploadBuffer;

		pCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}
}

D3D12_VIEWPORT NX12Util::ViewPort(float width, float height, float minDepth, float maxDepth, float topLeftX, float topLeftY)
{
	D3D12_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = topLeftX;
	vp.TopLeftY = topLeftY;
	return vp;
}

D3D12_CPU_DESCRIPTOR_HANDLE NX12Util::CPUDescriptorHandle(size_t ptr)
{
	return { ptr };
}

D3D12_ROOT_PARAMETER NX12Util::CreateRootParameterCBV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rp;
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rp.Descriptor.ShaderRegister = slot;
	rp.Descriptor.RegisterSpace = space;
	rp.ShaderVisibility = visibility;
	return rp;
}

D3D12_ROOT_PARAMETER NX12Util::CreateRootParameterSRV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rp;
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rp.Descriptor.ShaderRegister = slot;
	rp.Descriptor.RegisterSpace = space;
	rp.ShaderVisibility = visibility;
	return rp;
}

D3D12_ROOT_PARAMETER NX12Util::CreateRootParameterUAV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rp;
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rp.Descriptor.ShaderRegister = slot;
	rp.Descriptor.RegisterSpace = space;
	rp.ShaderVisibility = visibility;
	return rp;
}

D3D12_ROOT_PARAMETER NX12Util::CreateRootParameterTable(UINT numRanges, const D3D12_DESCRIPTOR_RANGE* pRanges, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rp;
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp.DescriptorTable.NumDescriptorRanges = numRanges;
	rp.DescriptorTable.pDescriptorRanges = pRanges;
	rp.ShaderVisibility = visibility;
	return rp;
}

D3D12_ROOT_PARAMETER NX12Util::CreateRootParameterTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& pRanges, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rp;
	rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp.DescriptorTable.NumDescriptorRanges = (UINT)pRanges.size();
	rp.DescriptorTable.pDescriptorRanges = pRanges.data();
	rp.ShaderVisibility = visibility;
	return rp;
}

D3D12_DESCRIPTOR_RANGE NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT slotStart, UINT space, UINT offset)
{
	D3D12_DESCRIPTOR_RANGE dr;
	dr.RangeType = rangeType;
	dr.NumDescriptors = numDescriptors;
	dr.BaseShaderRegister = slotStart;
	dr.RegisterSpace = space;
	dr.OffsetInDescriptorsFromTableStart = offset;
	return dr;
}

ID3D12RootSignature* NX12Util::CreateRootSignature(ID3D12Device* pDevice, UINT numParams, const D3D12_ROOT_PARAMETER* pParams, UINT numSamplers, const D3D12_STATIC_SAMPLER_DESC* pSamplers)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = numParams;
	desc.pParameters = pParams;
	desc.NumStaticSamplers = numSamplers;
	desc.pStaticSamplers = pSamplers;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> pSigBlob = nullptr;
	ComPtr<ID3DBlob> pErrorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
	if (FAILED(hr))
		return nullptr;

	ID3D12RootSignature* pRootSig = nullptr;
	hr = pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
	pSigBlob->Release();
	if (FAILED(hr))
	{
		pRootSig->Release();
		return nullptr;
	}

	return pRootSig;
}

ID3D12RootSignature* NX12Util::CreateRootSignature(ID3D12Device* pDevice, const std::vector<D3D12_ROOT_PARAMETER>& pParams, const std::vector<D3D12_STATIC_SAMPLER_DESC>& pSamplers)
{
	return CreateRootSignature(pDevice, (UINT)pParams.size(), pParams.data(), (UINT)pSamplers.size(), pSamplers.data());
}

UINT NX12Util::ByteAlign256(UINT sizeInBytes)
{
	return (sizeInBytes + 255) & ~255;
}

UINT NX12Util::GetRequiredIntermediateSize(ID3D12Device* pDevice, ID3D12Resource* pResource)
{
	D3D12_RESOURCE_DESC desc = pResource->GetDesc();
	UINT64 requiredSize = 0;
	pDevice->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, nullptr, nullptr, nullptr, &requiredSize);
	return (UINT)requiredSize;
}

UINT NX12Util::GetRequiredIntermediateLayoutInfos(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* oLayouts, UINT* oNumRows, UINT64* oRowSizeInBytes)
{
	D3D12_RESOURCE_DESC desc = pResource->GetDesc();
	UINT64 requiredSize = 0;
	pDevice->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, oLayouts, oNumRows, oRowSizeInBytes, &requiredSize);
	return (UINT)requiredSize;
}

void NX12Util::BeginEvent(ID3D12GraphicsCommandList* pCmdList, PCSTR fmt)
{
#ifdef DEBUG
	PIXBeginEvent(pCmdList, 0, fmt);
#endif // DEBUG
}

void NX12Util::EndEvent()
{
	PIXEndEvent();
}
