#pragma once

// 用于全局函数一次性定义。
// 平时开发时，不要引用这个类！

#if defined(DEBUG) | defined(_DEBUG)
    #undef DEBUG_NEW
    #undef new
#endif

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imfilebrowser.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#if defined(DEBUG) | defined(_DEBUG)
    #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW 
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
