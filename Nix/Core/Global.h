#pragma once
#include <Windows.h>
#include "BaseDefs/DX11.h"

class App;
class NXTimer;

// Global variables
extern	HINSTANCE				g_hInst;
extern	HWND					g_hWnd;

extern	ComPtr<ID3D11Device5>				g_pDevice;
extern	ComPtr<ID3D11DeviceContext4>		g_pContext;
extern	ComPtr<IDXGISwapChain4>				g_pSwapChain;
extern	ComPtr<ID3DUserDefinedAnnotation>	g_pUDA;

extern	App* g_app;
extern	NXTimer* g_timer;

