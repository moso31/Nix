#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#include <string>

class NX12Util
{
public:
	static ID3D12Resource* CreateBuffer(ID3D12Device* pDevice, const std::wstring& name, UINT sizeOfByte, D3D12_HEAP_TYPE heapType);
	static ID3D12Resource* CreateTexture2D(ID3D12Device* pDevice, const std::wstring& name, UINT width, UINT height, DXGI_FORMAT format, UINT mipLevels, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initState);
	static D3D12_RESOURCE_DESC CreateResourceDesc_DepthStencil(UINT width, UINT height, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);
	static D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE heapType);
	static D3D12_CLEAR_VALUE CreateClearValue(float depth = 1.0f, UINT8 stencil = 0, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);
	static D3D12_RESOURCE_BARRIER CreateResourceBarrier_Transition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

	static void CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT mipCount, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts);

	static UINT ByteAlign256(UINT sizeInBytes);
	static UINT GetRequiredIntermediateSize(ID3D12Device* pDevice, ID3D12Resource* pResource);
	static UINT GetRequiredIntermediateLayoutInfos(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* oLayouts, UINT* oNumRows, UINT64* oRowSizeInBytes);
};
