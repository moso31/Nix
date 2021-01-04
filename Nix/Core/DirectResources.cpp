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

	OnResize(width, height);
}

void DirectResources::OnResize(UINT width, UINT height)
{
	// ����ض�����һ���ڴ�С�������ġ�
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	g_pContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_pRTVOffScreen = nullptr;
	m_pRenderTargetView = nullptr;
	m_pDepthStencilView = nullptr;
	g_pContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	if (g_pSwapChain)
	{
		// ����������Ѵ��ڣ���������С��
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
		// �ݲ�ʹ�ö����
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		// ʹ��˫�������̶ȵؼ�С�ӳ�
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


	// ������������̨����������ȾĿ����ͼ��
	ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
	NX::ThrowIfFailed(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRenderTargetView));

	// ������Ҫ�������� 3D ��Ⱦ�����ģ����ͼ��
	CD3D11_TEXTURE2D_DESC descDepth(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(width),
		lround(height),
		1, // �����ģ����ͼֻ��һ������
		1, // ʹ�õ�һ mipmap ����
		D3D11_BIND_DEPTH_STENCIL
	);

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&descDepth, nullptr, &m_pTexDepthStencil));

	CD3D11_DEPTH_STENCIL_VIEW_DESC descDepthStencilView(D3D11_DSV_DIMENSION_TEXTURE2D);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexDepthStencil.Get(), &descDepthStencilView, &m_pDepthStencilView));

	// Create Render Target
	CD3D11_TEXTURE2D_DESC descOffScreen(
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		lround(width), 
		lround(height), 
		1, 
		1,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	g_pDevice->CreateTexture2D(&descOffScreen, nullptr, &m_pTexOffScreen);
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexOffScreen.Get(), nullptr, &m_pRTVOffScreen));
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexOffScreen.Get(), nullptr, &m_pSRVOffScreen));

	// Setup the viewport
	m_viewSize = { (FLOAT)width, (FLOAT)height };
	m_viewPort = CD3D11_VIEWPORT(0.0f, 0.0f, m_viewSize.x, m_viewSize.y);
	g_pContext->RSSetViewports(1, &m_viewPort);
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
