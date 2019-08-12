#pragma once

// 从 Windows 头文件中排除极少使用的内容
#define WIN32_LEAN_AND_MEAN

// Windows 
#include <windows.h>

// C 
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++/STL/11 
#include <memory>
#include <string>
#include <vector>
#include <map>

// DirectX
#include <d3d11_1.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

// Math
#include "SimpleMath.h"

// namespace
using namespace std;
using namespace DirectX;
using namespace SimpleMath;


// class preload
class App;
class DirectResources;

// Global variables
extern	HINSTANCE					g_hInst;
extern	HWND						g_hWnd;
extern	ID3D11Device*				g_pDevice;
extern	ID3D11Device1*				g_pDevice1;
extern	ID3D11DeviceContext*		g_pContext;
extern	ID3D11DeviceContext1*		g_pContext1;
extern	IDXGISwapChain*				g_pSwapChain;
extern	IDXGISwapChain1*			g_pSwapChain1;
extern	ID3D11RenderTargetView*		g_pRenderTargetView;

extern	shared_ptr<App>					g_app;
extern	shared_ptr<DirectResources>		g_dxResources;