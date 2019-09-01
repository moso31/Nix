#include "DirectResources.h"

void DirectResources::InitDevice()
{
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
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

	ID3D11Device* pDevice = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
		featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext);

	if (FAILED(hr))
	{
		NX::ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, 0, createDeviceFlags,
			featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext));
	}

	NX::ThrowIfFailed(pDevice->QueryInterface(__uuidof(ID3D11Device5), reinterpret_cast<void**>(&g_pDevice)));
	NX::ThrowIfFailed(pContext->QueryInterface(__uuidof(ID3D11DeviceContext4), reinterpret_cast<void**>(&g_pContext)));

	OnResize(width, height);
}

void DirectResources::OnResize(UINT width, UINT height)
{
	// 清除特定于上一窗口大小的上下文。
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	g_pContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_pRenderTargetView = nullptr;
	m_pDepthStencilView = nullptr;
	g_pContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	if (g_pSwapChain)
	{
		// 如果交换链已存在，请调整其大小。
		HRESULT hr = g_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Stereo = false;
		// 暂不使用多采样
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		// 使用双缓冲最大程度地减小延迟
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.Flags = 0;
		sd.Scaling = DXGI_SCALING_NONE;
		sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		IDXGIDevice4* pDxgiDevice;
		NX::ThrowIfFailed(g_pDevice->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&pDxgiDevice)));

		IDXGIAdapter* pDxgiAdapter;
		NX::ThrowIfFailed(pDxgiDevice->GetAdapter(&pDxgiAdapter));

		IDXGIFactory7* pDxgiFactory;
		NX::ThrowIfFailed(pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory)));

		IDXGISwapChain1* pSwapChain;
		NX::ThrowIfFailed(pDxgiFactory->CreateSwapChainForHwnd(g_pDevice, g_hWnd, &sd, nullptr, nullptr, &pSwapChain));
		
		NX::ThrowIfFailed(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), reinterpret_cast<void**>(&g_pSwapChain)));
	}


	// 创建交换链后台缓冲区的渲染目标视图。
	ID3D11Texture2D1* pBackBuffer = nullptr;
	NX::ThrowIfFailed(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView1(pBackBuffer, nullptr, &m_pRenderTargetView));
	pBackBuffer->Release();

	// 根据需要创建用于 3D 渲染的深度模具视图。
	CD3D11_TEXTURE2D_DESC1 descDepth(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(width),
		lround(height),
		1, // 此深度模具视图只有一个纹理。
		1, // 使用单一 mipmap 级别。
		D3D11_BIND_DEPTH_STENCIL
	);

	ID3D11Texture2D1* pDepthStencil;
	NX::ThrowIfFailed(g_pDevice->CreateTexture2D1(&descDepth, nullptr, &pDepthStencil));

	CD3D11_DEPTH_STENCIL_VIEW_DESC descDepthStencilView(D3D11_DSV_DIMENSION_TEXTURE2D);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(pDepthStencil, &descDepthStencilView, &m_pDepthStencilView));

	m_viewSize = { (FLOAT)width, (FLOAT)height };
	// Setup the viewport
	m_ViewPort = CD3D11_VIEWPORT(0.0f, 0.0f, m_viewSize.x, m_viewSize.y);
	g_pContext->RSSetViewports(1, &m_ViewPort);
}

void DirectResources::ClearDevices()
{
	if (g_pContext)				g_pContext->ClearState();

	if (g_pSwapChain)			g_pSwapChain->Release();
	if (g_pContext)				g_pContext->Release();
	if (g_pDevice)				g_pDevice->Release();
	if (m_pRenderTargetView)	m_pRenderTargetView->Release();
	if (m_pDepthStencilView)	m_pDepthStencilView->Release();
}

Vector2 DirectResources::GetViewSize()
{
	return m_viewSize;
}

Vector2 DirectResources::GetViewPortSize()
{
	return Vector2(m_ViewPort.Width, m_ViewPort.Height);
}
