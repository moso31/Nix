#pragma once

#include <Windows.h>
#include "BaseDefs/DX12.h"

#if defined(DEBUG) | defined(_DEBUG)
#undef DEBUG_NEW
#undef new
#endif

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"

#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW 
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Nix 的 imgui 使用的 字体集，需要在 NXGUI::Init() 中初始化它们。
extern ImFont* g_imgui_font_general;
extern ImFont* g_imgui_font_codeEditor;
