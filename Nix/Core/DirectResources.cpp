#include "DirectResources.h"
#include "BaseDefs/NixCore.h"
#include "Global.h"

void DirectResources::InitDevice()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif

	HRESULT hr;
	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pDXGIFactory));

	ComPtr<IDXGIAdapter1> pAdapter;
	m_pDXGIFactory->EnumAdapters1(0, &pAdapter);

	ComPtr<IDXGIAdapter4> pAdapter4;
	hr = pAdapter.As(&pAdapter4);

	hr = D3D12CreateDevice(pAdapter4.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&g_pDevice));
	hr = g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hr = g_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue));
	hr = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));
	hr = g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_pCommandList));

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;
	OnResize(width, height);
}

void DirectResources::InitCommandLists()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hr;

	// 创建命令队列
	hr = g_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue));

	// 创建命令分配器
	hr = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));

	// 创建命令列表，并将其和 命令分配器 关联
	// 同时设定初始渲染管线状态 = nullptr（默认状态）
	hr = g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_pCommandList));
}

void DirectResources::OnResize(UINT width, UINT height)
{
	HRESULT hr;

	if (m_pSwapChain.Get())
	{
		hr = m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 8 位 RGBA
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60; // 刷新率
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.SampleDesc.Count = 1; // MSAA4x 禁用
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 用于RT
		swapChainDesc.BufferCount = 2; // 双缓冲
		swapChainDesc.OutputWindow = g_hWnd; // 窗口句柄
		swapChainDesc.Windowed = true; // 窗口模式
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 翻转模式
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 允许全屏切换

		ComPtr<IDXGISwapChain> pSwapChain;
		hr = m_pDXGIFactory->CreateSwapChain(g_pCommandQueue.Get(), &swapChainDesc, &pSwapChain);
		hr = pSwapChain.As(&m_pSwapChain);

		// 构建描述符
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 2; // 双缓冲
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 无标志
		rtvHeapDesc.NodeMask = 0; // 单GPU

		// 创建描述符堆
		hr = g_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();

		// 创建RTV
		for (int i = 0; i < 2; ++i)
		{
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainRT[i]));
			g_pDevice->CreateRenderTargetView(m_pSwapChainRT[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			std::wstring strDebugName = L"SwapChain RT " + std::to_wstring(i);
			m_pSwapChainRT[i]->SetName(strDebugName.c_str());
		}
	}
}

void DirectResources::PrepareToRenderGUI()
{
	// 2023.12.10 设置最终呈现屏幕渲染结果所使用的RTV。
	// 目前在主渲染结束后，GUI开始渲染前执行这一步骤。GUI将会被逐个渲染到当前帧，SwapChain的RTV上。
	g_pCommandList->OMSetRenderTargets(1, &GetCurrentSwapChainRTV(), true, nullptr);
	g_pCommandList->ClearRenderTargetView(GetCurrentSwapChainRTV(), DirectX::Colors::Black, 0, nullptr);
}

void DirectResources::Release()
{
	// 2023.12.11 好像没啥用，先注掉，等跑通了再看
	//g_pCommandList->ClearState(nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectResources::GetCurrentSwapChainRTV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * m_pSwapChain->GetCurrentBackBufferIndex();
	return cpuHandle;
}
