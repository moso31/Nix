#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#define FRAME_BUFFER_NUM 3 // 使用三缓冲

template <typename T>
class MultiFrame
{
public:
	T& operator[](size_t index) { return data[index]; }
	const T& operator[](size_t index) const { return data[index]; }

	void Reset(const T& val) { for (int i = 0; i < FRAME_BUFFER_NUM; i++) data[i] = val; }

protected:
	T data[FRAME_BUFFER_NUM];
};

template <typename T>
struct XAllocatorData
{
	virtual UINT DataByteSize() { return sizeof(T); }

	T data;
	UINT pageIndex; // 记录该数据在 XAllocator 的页面索引
	UINT pageByteOffset; // 记录该数据在 XAllocator 的页面的字节偏移量
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddr; // 记录该数据的 GPU 虚拟地址
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
	static D3D12_RESOURCE_BARRIER CreateResourceBarrier_Transition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

	static void CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT layoutSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts);

	static UINT ByteAlign256(UINT sizeInBytes);
	static UINT GetRequiredIntermediateSize(ID3D12Device* pDevice, ID3D12Resource* pResource);
	static UINT GetRequiredIntermediateLayoutInfos(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* oLayouts, UINT* oNumRows, UINT64* oRowSizeInBytes);
};
