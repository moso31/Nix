#pragma once
//
//// 从 Windows 头文件中排除极少使用的内容
//#define WIN32_LEAN_AND_MEAN
//
//// Windows 
//#include <windows.h>
//#include <wrl.h>
//
//// C 
//#if defined(DEBUG) | defined(_DEBUG)
//	#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
//	#define new DEBUG_NEW 
//#endif
//
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//
//#include <malloc.h>
//#include <memory.h>
//#include <tchar.h>
//
//// C++/STL/
//#include <memory>
//#include <string>
//#include <vector>
//#include <list>
//#include <map>
//#include <set>
//#include <unordered_set>
//#include <chrono>
//#include <algorithm>
//#include <variant>
//#include <coroutine>
//#include <fstream>
//#include <filesystem>
//
//// DirectX
//#include <d3d11_4.h>
//#include <dxgi1_6.h>
//#include <DirectXColors.h>
//#include <d3dcompiler.h>
//
//// GUI
//#include "NXGUIImpl.h"
//
//// NX DirectX "helper"
//#include "NXDXHelper.h"
//
//// Math
//#include "SimpleMath.h"
//
//// using & namespace
//using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
//
//using namespace DirectX;
//using namespace DirectX::SimpleMath;
//using namespace Microsoft::WRL;
//
//#include "BasicDef.h"
//
//enum NXKeyCode
//{
//	LeftShift = 16,
//	LeftControl = 17,
//	LeftAlt = 18,
//};
//
//// class preload
//class App;
//class DirectResources;
//class NXInput;
//class NXEventManager;
//class NXScript;
//class NXTimer;
//
//// resource managers.
//class NXResourceManager;
//class NXCameraResourceManager;
//class NXLightResourceManager;
//class NXMaterialResourceManager;
//class NXMeshResourceManager;
//class NXScriptResourceManager;
//class NXTextureResourceManager;
//
//// serialization
//class NXSerializable;
//
//// resources
//struct TextureNXInfo;
//class NXTexture;
//class NXTexture2D;
//class NXTextureCube;
//class NXTexture2DArray;
//class NXBRDFLut;
//
//// scene
//class NXScene;
//class NXEditorObjectManager;
//
//// objects
//class NXObject;
//class NXTransform;
//class NXRenderableObject;
//class NXPrimitive;
//class NXPrefab;
//class NXCamera;
//class NXCubeMap;
//// submeshes
//class NXSubMeshBase;
//enum EditorObjectID;
//
//class NXMaterial;
//class NXCustomMaterial;
//
//class NXPBRLight;
//class NXPBRDistantLight;
//class NXPBRPointLight;
//class NXPBRSpotLight;
//
//// effects
//class NXRenderTarget;
//
//// post processing
//class NXSimpleSSAO;
//
//// Global variables
//extern	HINSTANCE				g_hInst;
//extern	HWND					g_hWnd;
//
//extern	ComPtr<ID3D11Device5>				g_pDevice;
//extern	ComPtr<ID3D11DeviceContext4>		g_pContext;
//extern	ComPtr<IDXGISwapChain4>				g_pSwapChain;
//extern	ComPtr<ID3DUserDefinedAnnotation>	g_pUDA;
//
//extern	App*					g_app;
//extern	NXTimer*				g_timer;
//
//const std::string   g_str_empty = "";
//
//// texture file paths
//const std::string	g_defaultTex_white_str = ".\\Resource\\white1x1.png";
//const std::string	g_defaultTex_normal_str = ".\\Resource\\normal1x1.png";
//const std::wstring	g_defaultTex_white_wstr = L".\\Resource\\white1x1.png";
//const std::wstring	g_defaultTex_normal_wstr = L".\\Resource\\normal1x1.png";
//
//// material templates
//const std::string   g_material_template_standardPBR = ".\\Resource\\shaders\\Default.nsl";