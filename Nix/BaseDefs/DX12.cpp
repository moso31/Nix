#include "DX12.h"

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

ID3D12Resource* NX12Util::CreateTexture2D(ID3D12Device* pDevice, const std::string& name, UINT width, UINT height, DXGI_FORMAT format, UINT mipLevels, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initState)
{
	ID3D12Resource* pResource;
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = width;
	desc.Height = height;
	desc.Format = format;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = mipLevels;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = pDevice->CreateCommittedResource(
		&CreateHeapProperties(heapType),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initState,
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
