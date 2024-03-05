#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "ShaderStructures.h"

class NXGlobalWindows
{
public:
	static HINSTANCE	hInst;
	static HWND			hWnd;
};

class NXGlobalDX
{
public:
	static void Init(IDXGIAdapter4* pAdapter);
	static ID3D12Device8* GetDevice() { return device.Get(); }
	static ID3D12CommandQueue* GetCmdQueue() { return cmdQueue.Get(); }
	static ID3D12GraphicsCommandList* CurrentCmdList() { return cmdList.Current().Get(); }
	static ID3D12CommandAllocator* CurrentCmdAllocator() { return cmdAllocator.Current().Get(); }

	static ComPtr<ID3D12Device8>							device;
	static ComPtr<ID3D12CommandQueue>						cmdQueue;
	static MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	cmdList;
	static MultiFrame<ComPtr<ID3D12CommandAllocator>>		cmdAllocator;
};

class App;
class NXTimer;
class NXGlobalApp
{
public:
	static App* App;
	static NXTimer* Timer;
};

// 2024.1.26 TODO：cbData按Object和View分可以，但现在的分法有问题。
// cbObject中存在一些view proj之类的参数，实际上应该放在Camera中。 // 将来再改，现在没空
class NXGlobalBuffer
{
public:
	static void Init();

	static MultiFrame<CommittedResourceData<ConstantBufferObject>>		cbObject;
	static MultiFrame<CommittedResourceData<ConstantBufferCamera>>		cbCamera;
	static MultiFrame<CommittedResourceData<ConstantBufferShadowTest>>	cbShadowTest;
};

class NXGlobalInputLayout
{
public:
	static void Init();

	static D3D12_INPUT_LAYOUT_DESC layoutP;
	static D3D12_INPUT_LAYOUT_DESC layoutPT;
	static D3D12_INPUT_LAYOUT_DESC layoutPNT;
	static D3D12_INPUT_LAYOUT_DESC layoutPNTT;
	static D3D12_INPUT_LAYOUT_DESC layoutEditorObject;
};
