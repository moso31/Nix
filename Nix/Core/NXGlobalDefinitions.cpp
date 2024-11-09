#include "NXGlobalDefinitions.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"

HINSTANCE	NXGlobalWindows::hInst;
HWND		NXGlobalWindows::hWnd;

ComPtr<ID3D12Device8>							NXGlobalDX::s_device;
ComPtr<ID3D12CommandQueue>						NXGlobalDX::s_cmdQueue;
MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	NXGlobalDX::s_cmdList;
MultiFrame<ComPtr<ID3D12CommandAllocator>>		NXGlobalDX::s_cmdAllocator;

void NXGlobalDX::Init(IDXGIAdapter4* pAdapter)
{
	HRESULT hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&s_device));
	s_cmdQueue = NX12Util::CreateCommandQueue(s_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, false);
	s_cmdQueue->SetName(L"Global Static Command Queue");

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		s_cmdAllocator[i] = NX12Util::CreateCommandAllocator(s_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		std::wstring strDbgName = L"Global Static Command Allocator " + std::to_wstring(i);
		s_cmdAllocator[i]->SetName(strDbgName.c_str());

		s_cmdList[i] = NX12Util::CreateGraphicsCommandList(s_device.Get(), s_cmdAllocator.Get(i).Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		strDbgName = L"Global Static Command List " + std::to_wstring(i);
		s_cmdList[i]->SetName(strDbgName.c_str());
	}
}

void NXGlobalDX::Release()
{
}

App*		NXGlobalApp::App;
NXTimer*	NXGlobalApp::Timer;

ConstantBufferObject NXGlobalBuffer::cbDataObject;
NXConstantBuffer<ConstantBufferObject>		NXGlobalBuffer::cbObject;
ConstantBufferCamera NXGlobalBuffer::cbDataCamera;
NXConstantBuffer<ConstantBufferCamera>		NXGlobalBuffer::cbCamera;
ConstantBufferShadowTest NXGlobalBuffer::cbDataShadowTest;
NXConstantBuffer<ConstantBufferShadowTest>	NXGlobalBuffer::cbShadowTest;

void NXGlobalBuffer::Init()
{
}

D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutP;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPNT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutPNTT;
D3D12_INPUT_LAYOUT_DESC	NXGlobalInputLayout::layoutEditorObject;

void NXGlobalInputLayout::Init()
{
	static D3D12_INPUT_ELEMENT_DESC layoutDescP[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	layoutP = { layoutDescP, _countof(layoutDescP)};

	static D3D12_INPUT_ELEMENT_DESC layoutDescPT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPT = { layoutDescPT, _countof(layoutDescPT) };

	static D3D12_INPUT_ELEMENT_DESC layoutDescPNT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPNT = { layoutDescPNT, _countof(layoutDescPNT) };

	static D3D12_INPUT_ELEMENT_DESC layoutDescPNTT[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutPNTT = { layoutDescPNTT, _countof(layoutDescPNTT) };

	static D3D12_INPUT_ELEMENT_DESC layoutDescEditorObject[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	layoutEditorObject = { layoutDescEditorObject, _countof(layoutDescEditorObject) };
}
