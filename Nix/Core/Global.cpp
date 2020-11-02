#include "Header.h"

HINSTANCE						g_hInst = nullptr;
HWND							g_hWnd = nullptr;
ID3D11Device5*					g_pDevice = nullptr;
ID3D11DeviceContext4*			g_pContext = nullptr;
IDXGISwapChain4*				g_pSwapChain = nullptr;

App*						g_app = nullptr;
DirectResources*			g_dxResources = nullptr;
NXTimer*					g_timer = nullptr;
