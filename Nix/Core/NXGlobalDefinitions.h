#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "ShaderStructures.h"
#include "NXConstantBuffer.h"
#include "NXStructuredBuffer.h"

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
	static ID3D12Device8* GetDevice() { return s_device.Get(); }
	static ID3D12CommandQueue* GetCmdQueue() { return s_cmdQueue.Get(); }
	static ID3D12GraphicsCommandList* CurrentCmdList() { return s_cmdList.Current().Get(); }
	static ID3D12CommandAllocator* CurrentCmdAllocator() { return s_cmdAllocator.Current().Get(); }
	static void Release();

	static ComPtr<ID3D12Device8>							s_device;
	static ComPtr<ID3D12CommandQueue>						s_cmdQueue;
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
};

// 2024.1.26 TODO��cbData��Object��View�ֿ��ԣ������ڵķַ������⡣
// cbObject�д���һЩview proj֮��Ĳ�����ʵ����Ӧ�÷���Camera�С� // �����ٸģ�����û��
class NXGlobalBuffer
{
public:
	static void Init();

	static NXConstantBuffer<ConstantBufferObject>		cbObject;
	static NXConstantBuffer<ConstantBufferCamera>		cbCamera;
	static NXConstantBuffer<ConstantBufferShadowTest>	cbShadowTest;
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
