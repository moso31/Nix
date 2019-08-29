#include "Header.h"

HINSTANCE						g_hInst = nullptr;
HWND							g_hWnd = nullptr;
ID3D11Device5*					g_pDevice = nullptr;
ID3D11DeviceContext4*			g_pContext = nullptr;
IDXGISwapChain4*				g_pSwapChain = nullptr;

shared_ptr<App>					g_app;
shared_ptr<DirectResources>		g_dxResources = nullptr;
shared_ptr<NXTimer>				g_timer = nullptr;