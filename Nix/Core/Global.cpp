#include "Header.h"

HINSTANCE							g_hInst = nullptr;
HWND								g_hWnd = nullptr;
ComPtr<ID3D11Device5>				g_pDevice = nullptr;
ComPtr<ID3D11DeviceContext4>		g_pContext = nullptr;
ComPtr<IDXGISwapChain4>				g_pSwapChain = nullptr;
ComPtr<ID3DUserDefinedAnnotation>	g_pUDA = nullptr;

App*						g_app = nullptr;
DirectResources*			g_dxResources = nullptr;
NXTimer*					g_timer = nullptr;
