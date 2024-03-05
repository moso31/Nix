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
	HRESULT hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
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

D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutP;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPNT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPNTT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutEditorObject;

void NXGlobalInputLayout::Init()
{
	D3D12_INPUT_ELEMENT_DESC layoutDescP[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	layoutP = { layoutDescP, _countof(layoutDescP)};

	D3D12_INPUT_ELEMENT_DESC layoutDescPT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPT = { layoutDescPT, _countof(layoutDescPT) };

	D3D12_INPUT_ELEMENT_DESC layoutDescPNT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPNT = { layoutDescPNT, _countof(layoutDescPNT) };

	D3D12_INPUT_ELEMENT_DESC layoutDescPNTT[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPNTT = { layoutDescPNTT, _countof(layoutDescPNTT) };

	D3D12_INPUT_ELEMENT_DESC layoutDescEditorObject[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutEditorObject = { layoutDescEditorObject, _countof(layoutDescEditorObject) };
}
