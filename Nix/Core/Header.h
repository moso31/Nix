#pragma once

// 从 Windows 头文件中排除极少使用的内容
#define WIN32_LEAN_AND_MEAN

// Windows 
#include <windows.h>
#include <wrl.h>

// C 
#if defined(DEBUG) | defined(_DEBUG)
	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
	#define new DEBUG_NEW 
#endif

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++/STL/11 
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <chrono>
#include <algorithm>

// DirectX
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

// GUI
#include "NXGUIImpl.h"

// NX DirectX "helper"
#include "NXDXHelper.h"

// Math
#include "SimpleMath.h"

// using & namespace
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

#define SafeDeleteArray(x) { delete[] x; x = nullptr; }
#define SafeDelete(x) { delete x; x = nullptr; }
#define SafeReleaseCOM(x) { if (x) { x->Release(); x = nullptr; } }
#define SafeRelease(x) { if (x) { x->Release(); SafeDelete(x); } }

// class preload
class App;
class DirectResources;
class NXInput;
class NXEventManager;
class NXScript;
class NXTimer;

// resources
class NXResourceManager;
class NXTexture2D;

class NXScene;
class SceneManager;
class NXObject;
class NXPrimitive;
class NXCamera;
class NXCubeMap;
class NXSubMesh;

class NXMaterial;
class NXPBRMaterialBase;
class NXPBRMaterialStandard;
class NXPBRMaterialTranslucent;

class NXPBRLight;

// effects
class NXPassShadowMap;
class NXRenderTarget;

// post processing
class NXSimpleSSAO;

// Global variables
extern	HINSTANCE				g_hInst;
extern	HWND					g_hWnd;

extern	ComPtr<ID3D11Device5>				g_pDevice;
extern	ComPtr<ID3D11DeviceContext4>		g_pContext;
extern	ComPtr<IDXGISwapChain4>				g_pSwapChain;
extern	ComPtr<ID3DUserDefinedAnnotation>	g_pUDA;

extern	App*					g_app;
extern	DirectResources*		g_dxResources;
extern	NXTimer*				g_timer;
