#pragma once
#include <Windows.h>
#include "BaseDefs/DX12.h"

class App;
class NXTimer;

// Global variables
extern	HINSTANCE				g_hInst;
extern	HWND					g_hWnd;

extern	ComPtr<ID3D12Device8>				g_pDevice;
extern	ComPtr<ID3D12CommandQueue>			g_pCommandQueue;
extern	ComPtr<ID3D12CommandAllocator>		g_pCommandAllocator;
extern	ComPtr<ID3D12GraphicsCommandList6>	g_pCommandList;

extern	App* g_app;
extern	NXTimer* g_timer;

