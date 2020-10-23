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
#include <list>
#include <map>
#include <chrono>
#include <algorithm>

// DirectX
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

// NX DirectX "helper"
#include "NXDXHelper.h"

// Math
#include "SimpleMath.h"

// using & namespace
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

using namespace DirectX;
using namespace DirectX::SimpleMath;

// class preload
class App;
class DirectResources;
class NXGlobalBufferManager;
class NXInput;
struct NXEventArg;
class NXScript;
class NXTimer;

class NXScene;
class SceneManager;
//class HBVHTree;	// 暂时先不用这个
class NXObject;
class NXPrimitive;
class NXCamera;
class NXCubeMap;

// effects
class NXPassShadowMap;
class NXRenderTarget;

// Global variables
extern	HINSTANCE					g_hInst;
extern	HWND						g_hWnd;
extern	ID3D11Device5*				g_pDevice;
extern	ID3D11DeviceContext4*		g_pContext;
extern	IDXGISwapChain4*			g_pSwapChain;

extern	std::shared_ptr<App>					g_app;
extern	std::shared_ptr<DirectResources>		g_dxResources;
extern	std::shared_ptr<NXTimer>				g_timer;