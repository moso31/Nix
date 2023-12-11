#include "DX12.h"

ID3D12Resource* NX12Util::CreateResource_CBuffer(ID3D12Device* pDevice, UINT sizeOfByte, const std::wstring& name)
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
		&CreateHeapProperties_Upload(),
		D3D12_HEAP_FLAG_NONE,
		&desc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初始的资源状态为READ（允许CPU写入数据）
		nullptr,
		IID_PPV_ARGS(&pResource)
	);

	pResource->SetName(name.c_str());

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

D3D12_HEAP_PROPERTIES NX12Util::CreateHeapProperties_Upload()
{
	D3D12_HEAP_PROPERTIES hp;
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;
	return hp;
}

D3D12_HEAP_PROPERTIES NX12Util::CreateHeapProperties_Default()
{
	D3D12_HEAP_PROPERTIES hp;
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
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

UINT NX12Util::ByteAlign256(UINT sizeInBytes)
{
	return (sizeInBytes + 255) & ~255;
}
