#include "DirectResources.h"

HRESULT DirectResources::InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	ID3D11Device* pDevice = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	for (UINT driverTypeId = 0; driverTypeId < ARRAYSIZE(driverTypes); driverTypeId++)
	{
		D3D_DRIVER_TYPE driverType = driverTypes[driverTypeId];

		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext);

		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr)) 
		return hr;
	hr = pDevice->QueryInterface(__uuidof(ID3D11Device5), reinterpret_cast<void**>(&g_pDevice));
	if (FAILED(hr)) 
		return hr;
	hr = pContext->QueryInterface(__uuidof(ID3D11DeviceContext4), reinterpret_cast<void**>(&g_pContext));
	if (FAILED(hr)) 
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory5* dxgiFactory = nullptr;
	{
		IDXGIDevice4* dxgiDevice = nullptr;
		hr = g_pDevice->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* temp;
			IDXGIAdapter3* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&temp);
			temp->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&adapter));
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	if (dxgiFactory)
	{
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		IDXGISwapChain1* pSwapChain;
		hr = dxgiFactory->CreateSwapChainForHwnd(g_pDevice, g_hWnd, &sd, nullptr, nullptr, &pSwapChain);

		if (SUCCEEDED(hr))
		{
			hr = pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&g_pSwapChain));
		}
	}

	// block the ALT+ENTER shortcut
	//dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pDevice->CreateTexture2D(&descDepth, nullptr, &m_pDepthStencil);
	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pDevice->CreateDepthStencilView(m_pDepthStencil, &descDSV, &m_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	// Setup the viewport
	m_ViewPort.Width = (FLOAT)width;
	m_ViewPort.Height = (FLOAT)height;
	m_ViewPort.MinDepth = 0.0f;
	m_ViewPort.MaxDepth = 1.0f;
	m_ViewPort.TopLeftX = 0;
	m_ViewPort.TopLeftY = 0;
	g_pContext->RSSetViewports(1, &m_ViewPort);

	return S_OK;
}

void DirectResources::ClearDevices()
{
	if (g_pContext)				g_pContext->ClearState();

	if (g_pSwapChain)			g_pSwapChain->Release();
	if (g_pContext)				g_pContext->Release();
	if (g_pDevice)				g_pDevice->Release();
	if (m_pRenderTargetView)	m_pRenderTargetView->Release();
	if (m_pDepthStencil)		m_pDepthStencil->Release();
	if (m_pDepthStencilView)	m_pDepthStencilView->Release();
}

Vector2 DirectResources::GetViewPortSize()
{
	return Vector2(m_ViewPort.Width, m_ViewPort.Height);
}
