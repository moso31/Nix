#include "Header.h"

HINSTANCE						g_hInst = nullptr;
HWND							g_hWnd = nullptr;
ID3D11Device*					g_pDevice = nullptr;
ID3D11Device1*					g_pDevice1 = nullptr;
ID3D11DeviceContext*			g_pContext = nullptr;
ID3D11DeviceContext1*			g_pContext1 = nullptr;
IDXGISwapChain*					g_pSwapChain = nullptr;
IDXGISwapChain1*				g_pSwapChain1 = nullptr;
ID3D11RenderTargetView*			g_pRenderTargetView = nullptr;

shared_ptr<App>					g_app;
shared_ptr<DirectResources>		g_dxResources = nullptr;