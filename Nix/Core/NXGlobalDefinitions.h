#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <atomic>

class NXGlobalWindows
{
public:
	static HINSTANCE	hInst;
	static HWND			hWnd;
};

class NXGlobalDX
{
public:
	static void Init(IDXGIAdapter1* pAdapter);
	static ID3D12Device* GetDevice() { return s_device.Get(); }
	static ID3D12CommandQueue* GlobalCmdQueue() { return s_globalCmdQueue.Get(); }
	static ID3D12GraphicsCommandList* CurrentCmdList() { return s_cmdList.Current().Get(); }
	static ID3D12CommandAllocator* CurrentCmdAllocator() { return s_cmdAllocator.Current().Get(); }

	static ComPtr<ID3D12Device>							s_device;
	static ComPtr<ID3D12CommandQueue>						s_globalCmdQueue;
	static ComPtr<ID3D12Fence>								s_globalfence;
	static UINT64											s_globalfenceValue;
	static MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	s_cmdList;
	static MultiFrame<ComPtr<ID3D12CommandAllocator>>		s_cmdAllocator;
};

class App;
class NXTimer;
class NXGlobalApp
{
public:
	static App* App;
	static NXTimer* Timer;
	static std::atomic<uint64_t> s_frameIndex;
};

class NXGlobalInputLayout
{
public:
	static void Init();

	static D3D12_INPUT_LAYOUT_DESC layoutP;
	static D3D12_INPUT_LAYOUT_DESC layoutPT;
	static D3D12_INPUT_LAYOUT_DESC layoutPNT;
	static D3D12_INPUT_LAYOUT_DESC layoutPNTT;
	static D3D12_INPUT_LAYOUT_DESC layoutPNTT_GPUInstancing;
	static D3D12_INPUT_LAYOUT_DESC layoutEditorObject;
};

extern bool g_debug_temporal_enable_terrain_debug;