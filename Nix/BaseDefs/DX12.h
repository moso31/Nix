#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

#ifdef DEBUG
#include <pix.h>
#endif

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

#include <vector>
#include <string>

class NX12Util
{
public:
	static void CreateCommands(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandQueue* oCmdQueue, ID3D12CommandAllocator* oCmdAllocator, ID3D12GraphicsCommandList* oCmdList, bool disableGPUTimeOut = false);
	static ID3D12CommandQueue* CreateCommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
	static ID3D12CommandList* CreateCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* oCmdAllocator, D3D12_COMMAND_LIST_TYPE type, UINT nodeMask = 0, ID3D12PipelineState* InitState = nullptr);

	static ID3D12Resource* CreateBuffer(ID3D12Device* pDevice, const std::string& name, UINT sizeOfByte, D3D12_HEAP_TYPE heapType);
	static D3D12_RESOURCE_DESC CreateResourceDesc_DepthStencil(UINT width, UINT height, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);
	static D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE heapType);
	static D3D12_CLEAR_VALUE CreateClearValue(float depth = 1.0f, UINT8 stencil = 0, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);

	static void SetResourceBarrier(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
	static void CopyTextureRegion(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pTexture, ID3D12Resource* pTextureUploadBuffer, UINT layoutSize, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts);

	static D3D12_VIEWPORT ViewPort(float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f, float topLeftX = 0.0f, float topLeftY = 0.0f);

	static D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle(size_t ptr);

	static D3D12_ROOT_PARAMETER CreateRootParameterCBV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility);
	static D3D12_ROOT_PARAMETER CreateRootParameterSRV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility);
	static D3D12_ROOT_PARAMETER CreateRootParameterUAV(UINT slot, UINT space, D3D12_SHADER_VISIBILITY visibility);
	static D3D12_ROOT_PARAMETER CreateRootParameterTable(UINT numRanges, const D3D12_DESCRIPTOR_RANGE* pRanges, D3D12_SHADER_VISIBILITY visibility);
	static D3D12_ROOT_PARAMETER CreateRootParameterTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& pRanges, D3D12_SHADER_VISIBILITY visibility);

	static D3D12_DESCRIPTOR_RANGE CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT slotStart, UINT space = 0, UINT offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	static ID3D12RootSignature* CreateRootSignature(ID3D12Device* pDevice, UINT numParams, const D3D12_ROOT_PARAMETER* pParams, UINT numSamplers, const D3D12_STATIC_SAMPLER_DESC* pSamplers);
	static ID3D12RootSignature* CreateRootSignature(ID3D12Device* pDevice, const std::vector<D3D12_ROOT_PARAMETER>& pParams, const std::vector<D3D12_STATIC_SAMPLER_DESC>& pSamplers);
	static ID3D12RootSignature* CreateRootSignature(ID3D12Device* pDevice, const std::vector<D3D12_ROOT_PARAMETER>& pParams);

	static UINT ByteAlign256(UINT sizeInBytes);
	static UINT GetRequiredIntermediateSize(ID3D12Device* pDevice, ID3D12Resource* pResource);
	static UINT GetRequiredIntermediateLayoutInfos(ID3D12Device* pDevice, ID3D12Resource* pResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* oLayouts, UINT* oNumRows, UINT64* oRowSizeInBytes);

	static void BeginEvent(ID3D12GraphicsCommandList* pCmdList, PCSTR fmt);
	static void EndEvent();
};
