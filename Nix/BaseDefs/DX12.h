#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#define MultiFrameSets_swapChainCount 3 // 使用三缓冲

class MultiFrameSets
{
public:
	static UINT8 swapChainIndex; // 静态变量，用于存储当前帧的索引
};

UINT8 MultiFrameSets::swapChainIndex = 0; // 初始化静态变量

template <typename T>
class MultiFrame
{
public:
	T& operator[](size_t index) { return data[index]; }
	const T& operator[](size_t index) const { return data[index]; }

	void Reset(const T& val) { for (int i = 0; i < MultiFrameSets_swapChainCount; i++) data[i] = val; }
	T& Current() { return data[MultiFrameSets::swapChainIndex]; }
	T& Get(UINT8 index) { return data[index]; }

protected:
	T data[MultiFrameSets_swapChainCount];
};

#include <string>

class NX12Util
{
public:
	static ID3D12Resource* CreateBuffer(ID3D12Device* pDevice, const std::string& name, UINT sizeOfByte, D3D12_HEAP_TYPE heapType);
	static ID3D12Resource* CreateTexture2D(ID3D12Device* pDevice, const std::string& name, UINT width, UINT height, DXGI_FORMAT format, UINT mipLevels, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initState);
	static D3D12_RESOURCE_DESC CreateResourceDesc_DepthStencil(UINT width, UINT height, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);
	static D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE heapType);
	static D3D12_CLEAR_VALUE CreateClearValue(float depth = 1.0f, UINT8 stencil = 0, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);

	static void SetResourceBarrier(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
	static void CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT layoutSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts);

	static UINT ByteAlign256(UINT sizeInBytes);
	static UINT GetRequiredIntermediateSize(ID3D12Device* pDevice, ID3D12Resource* pResource);
	static UINT GetRequiredIntermediateLayoutInfos(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* oLayouts, UINT* oNumRows, UINT64* oRowSizeInBytes);
};
