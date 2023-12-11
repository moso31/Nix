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
	static ID3D12Resource* CreateResource_CBuffer(ID3D12Device* pDevice, UINT sizeOfByte, const std::wstring& name);
	static D3D12_RESOURCE_DESC CreateResourceDesc_DepthStencil(UINT width, UINT height, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);

	static D3D12_HEAP_PROPERTIES CreateHeapProperties_Upload();
	static D3D12_HEAP_PROPERTIES CreateHeapProperties_Default();

	static D3D12_CLEAR_VALUE CreateClearValue(float depth = 1.0f, UINT8 stencil = 0, DXGI_FORMAT fmt = DXGI_FORMAT_D24_UNORM_S8_UINT);

	static UINT ByteAlign256(UINT sizeInBytes);
};
