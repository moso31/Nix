#include "DX12.h"

UINT8 MultiFrameSets::swapChainIndex = 0; // 初始化静态变量

void NX12Util::CreateCommands(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandQueue** oCmdQueue, ID3D12CommandAllocator** oCmdAllocator, ID3D12GraphicsCommandList** oCmdList, bool disableGPUTimeOut, bool closeCmdListAtFirst)
{
	*oCmdQueue = CreateCommandQueue(pDevice, type, disableGPUTimeOut);
	CreateCommands(pDevice, type, oCmdAllocator, oCmdList, closeCmdListAtFirst);
}

void NX12Util::CreateCommands(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator** oCmdAllocator, ID3D12GraphicsCommandList** oCmdList, bool closeCmdListAtFirst)
{
	HRESULT hr;
	hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(oCmdAllocator));
	hr = pDevice->CreateCommandList(0, type, *oCmdAllocator, nullptr, IID_PPV_ARGS(oCmdList));

	if (closeCmdListAtFirst) (*oCmdList)->Close();
}

ID3D12CommandQueue* NX12Util::CreateCommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, bool disableGPUTimeOut)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Flags = disableGPUTimeOut ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;

	ID3D12CommandQueue* pCmdQueue;
	HRESULT hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCmdQueue));
	return pCmdQueue;
}

ID3D12CommandAllocator* NX12Util::CreateCommandAllocator(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	ID3D12CommandAllocator* pCmdAllocator;
	HRESULT hr = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pCmdAllocator));
	return pCmdAllocator;
}

ID3D12GraphicsCommandList* NX12Util::CreateGraphicsCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* pCmdAllocator, D3D12_COMMAND_LIST_TYPE type, UINT nodeMask, ID3D12PipelineState* InitState, bool closeCmdListAtFirst)
{
	ID3D12GraphicsCommandList* pCmdList;
	HRESULT hr = pDevice->CreateCommandList(nodeMask, type, pCmdAllocator, InitState, IID_PPV_ARGS(&pCmdList));
	if (closeCmdListAtFirst) pCmdList->Close();
	return pCmdList;
}

ID3D12Fence* NX12Util::CreateFence(ID3D12Device* pDevice, const std::wstring& errInfo)
{
	ID3D12Fence* pFence;
	HRESULT hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
	if (FAILED(hr))
	{
		MessageBox(NULL, errInfo.c_str(), L"Error", MB_OK);
	}

	return pFence;
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
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = pDevice->CreateCommittedResource(
		&CreateHeapProperties(heapType),
		D3D12_HEAP_FLAG_NONE,
		&desc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, 
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

void NX12Util::CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT layoutSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts)
{
	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.pResource = pTexture;

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	src.pResource = pTextureUploadBuffer;

	for (UINT idx = 0; idx < layoutSize; ++idx)
	{
		src.PlacedFootprint = pLayouts[idx];
		dst.SubresourceIndex = idx;

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

D3D12_RECT NX12Util::ScissorRect(const D3D12_VIEWPORT& vp)
{
	D3D12_RECT rect;
	rect.left = (LONG)vp.TopLeftX;
	rect.top = (LONG)vp.TopLeftY;
	rect.right = (LONG)(vp.TopLeftX + vp.Width);
	rect.bottom = (LONG)(vp.TopLeftY + vp.Height);
	return rect;
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
	{
		if (pErrorBlob != nullptr) 
		{
			// 将错误信息转换为字符指针
			char* errorMessage = static_cast<char*>(pErrorBlob->GetBufferPointer());

			// 打印错误信息
			printf("Error compiling root signature: %s\n", errorMessage);
		}
		return nullptr;
	}

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

ID3D12RootSignature* NX12Util::CreateRootSignature(ID3D12Device* pDevice, const std::vector<D3D12_ROOT_PARAMETER>& pParams)
{
	return CreateRootSignature(pDevice, (UINT)pParams.size(), pParams.data(), 0, nullptr);
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

void NX12Util::Transition(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDstResource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = { };
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = pDstResource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCmdList->ResourceBarrier(1, &barrier);
}

void NX12Util::BeginEvent(ID3D12GraphicsCommandList* pCmdList, PCSTR fmt)
{
#ifdef DEBUG
	PIXBeginEvent(pCmdList, 0, fmt);
#endif // DEBUG
}

void NX12Util::EndEvent(ID3D12GraphicsCommandList* pCmdList)
{
#ifdef DEBUG
	PIXEndEvent(pCmdList);
#endif // DEBUG
}
