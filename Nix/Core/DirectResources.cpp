#include "DirectResources.h"

void DirectResources::InitDevice()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
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

	ComPtr<ID3D11Device> pDevice;
	ComPtr<ID3D11DeviceContext> pContext;
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
		featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext);

	if (FAILED(hr))
	{
		NX::ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, 0, createDeviceFlags,
			featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &pDevice, &featureLevel, &pContext));
	}

	NX::ThrowIfFailed(pDevice.As(&g_pDevice));
	NX::ThrowIfFailed(pContext.As(&g_pContext));
	NX::ThrowIfFailed(pContext.As(&g_pUDA));

	ComPtr<ID3D11Debug> pDebug;
	if (SUCCEEDED(pDevice.As(&pDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(pDebug.As(&d3dInfoQueue)))
		{
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,

				// 在此添加想要禁用的 D3D11_MESSAGE_ID
				// Add more message IDs here as needed
				D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
			};

			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}

	OnResize(width, height);
}

void DirectResources::OnResize(UINT width, UINT height)
{
	// 清除特定于上一窗口大小的上下文。
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	g_pContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	g_pContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);
	m_pRTVSwapChainBuffer = nullptr;

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

		ComPtr<IDXGIDevice3> pDxgiDevice;
		NX::ThrowIfFailed(g_pDevice.As(&pDxgiDevice));

		ComPtr<IDXGIAdapter> pDxgiAdapter;
		NX::ThrowIfFailed(pDxgiDevice->GetAdapter(&pDxgiAdapter));

		ComPtr<IDXGIFactory5> pDxgiFactory;
		NX::ThrowIfFailed(pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory)));

		ComPtr<IDXGISwapChain1> pSwapChain;
		NX::ThrowIfFailed(pDxgiFactory->CreateSwapChainForHwnd(g_pDevice.Get(), g_hWnd, &sd, nullptr, nullptr, &pSwapChain));
		
		NX::ThrowIfFailed(pSwapChain.As(&g_pSwapChain));
	}


	// 创建交换链后台缓冲区的渲染目标视图。
	ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
	NX::ThrowIfFailed(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTVSwapChainBuffer));

	// Setup the viewport
	m_viewSize = { (FLOAT)width, (FLOAT)height };
	m_viewPort = CD3D11_VIEWPORT(0.0f, 0.0f, m_viewSize.x, m_viewSize.y);
	g_pContext->RSSetViewports(1, &m_viewPort);
}

void DirectResources::PrepareToRenderGUI()
{
	if (!m_pRTVSwapChainBuffer)
		return;

	// 2023.6.3 设置最终呈现屏幕渲染结果所使用的RTV。
	// 目前需要在主渲染结束后，GUI开始渲染前执行这一步骤。
	// 换句话说，在执行此方法后，GUI将会被逐个渲染到这个Buffer上。
	g_pContext->OMSetRenderTargets(1, m_pRTVSwapChainBuffer.GetAddressOf(), nullptr);
	g_pContext->ClearRenderTargetView(m_pRTVSwapChainBuffer.Get(), Colors::Black);
}

void DirectResources::Release()
{
	if (g_pContext)				
		g_pContext->ClearState();
}

Vector2 DirectResources::GetViewSize()
{
	return m_viewSize;
}

Vector2 DirectResources::GetViewPortSize()
{
	return Vector2(m_viewPort.Width, m_viewPort.Height);
}
