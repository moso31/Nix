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
#include <d3d11_4.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

// NX DirectX "helper"
#include "NXDXHelper.h"

// Math
#include "SimpleMath.h"
#include "SimpleMath.inl"

// namespace
using namespace std;
using namespace DirectX;
using namespace SimpleMath;

// class preload
class App;
class DirectResources;
class NXInput;
struct NXEventArg;

class Scene;
class NXObject;
class NXPrimitive;
class NXCamera;
class NXDirectionalLight;
class NXMaterial;

// Global variables
extern	HINSTANCE					g_hInst;
extern	HWND						g_hWnd;
extern	ID3D11Device5*				g_pDevice;
extern	ID3D11DeviceContext4*		g_pContext;
extern	IDXGISwapChain4*			g_pSwapChain;

extern	shared_ptr<App>					g_app;
extern	shared_ptr<DirectResources>		g_dxResources;