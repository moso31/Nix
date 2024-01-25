#include "Global.h"

HINSTANCE							g_hInst = nullptr;
HWND								g_hWnd = nullptr;

ComPtr<ID3D12Device8>				g_pDevice = nullptr;
ComPtr<ID3D12CommandQueue>			g_pCommandQueue = nullptr;

App*								g_app = nullptr;
NXTimer*							g_timer = nullptr;
