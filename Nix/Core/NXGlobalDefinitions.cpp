#include "NXGlobalDefinitions.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"

HINSTANCE	NXGlobalWindows::hInst;
HWND		NXGlobalWindows::hWnd;

ComPtr<ID3D12Device8>							NXGlobalDX::device;
ComPtr<ID3D12CommandQueue>						NXGlobalDX::cmdQueue;
MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	NXGlobalDX::cmdList;
MultiFrame<ComPtr<ID3D12CommandAllocator>>		NXGlobalDX::cmdAllocator;

void NXGlobalDX::Init(IDXGIAdapter4* pAdapter)
{
	HRESULT hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
	cmdQueue = NX12Util::CreateCommandQueue(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, false);

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		cmdAllocator[i] = NX12Util::CreateCommandAllocator(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		cmdList[i] = NX12Util::CreateCommandList(device.Get(), cmdAllocator.Get(i).Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
}

App*		NXGlobalApp::App;
NXTimer*	NXGlobalApp::Timer;

MultiFrame<CommittedResourceData<ConstantBufferObject>>		NXGlobalBuffer::cbObject;
MultiFrame<CommittedResourceData<ConstantBufferCamera>>		NXGlobalBuffer::cbCamera;
MultiFrame<CommittedResourceData<ConstantBufferShadowTest>>	NXGlobalBuffer::cbShadowTest;

void NXGlobalBuffer::Init()
{
	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, cbObject.Get(i));
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, cbCamera.Get(i));
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, cbShadowTest.Get(i));
	}
}

D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutP[1];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPT[2];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNT[3];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutPNTT[4];
D3D12_INPUT_ELEMENT_DESC	NXGlobalInputLayout::layoutEditorObject[2];

void NXGlobalInputLayout::Init()
{
	layoutP[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPT[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPNT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutPNTT[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutPNTT[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	layoutEditorObject[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	layoutEditorObject[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
}
