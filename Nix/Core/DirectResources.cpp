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

	// �����������
	hr = g_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pCommandQueue));

	// �������������
	hr = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));

	// ���������б�������� ��������� ����
	// ͬʱ�趨��ʼ��Ⱦ����״̬ = nullptr��Ĭ��״̬��
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
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 8 λ RGBA
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60; // ˢ����
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.SampleDesc.Count = 1; // MSAA4x ����
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ����RT
		swapChainDesc.BufferCount = 2; // ˫����
		swapChainDesc.OutputWindow = g_hWnd; // ���ھ��
		swapChainDesc.Windowed = true; // ����ģʽ
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ��תģʽ
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ����ȫ���л�

		ComPtr<IDXGISwapChain> pSwapChain;
		hr = m_pDXGIFactory->CreateSwapChain(g_pCommandQueue.Get(), &swapChainDesc, &pSwapChain);
		hr = pSwapChain.As(&m_pSwapChain);

		// ����������
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 2; // ˫����
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // �ޱ�־
		rtvHeapDesc.NodeMask = 0; // ��GPU

		// ������������
		hr = g_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();

		// ����RTV
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
	// 2023.12.10 �������ճ�����Ļ��Ⱦ�����ʹ�õ�RTV��
	// Ŀǰ������Ⱦ������GUI��ʼ��Ⱦǰִ����һ���衣GUI���ᱻ�����Ⱦ����ǰ֡��SwapChain��RTV�ϡ�
	g_pCommandList->OMSetRenderTargets(1, &GetCurrentSwapChainRTV(), true, nullptr);
	g_pCommandList->ClearRenderTargetView(GetCurrentSwapChainRTV(), DirectX::Colors::Black, 0, nullptr);
}

void DirectResources::Release()
{
	// 2023.12.11 ����ûɶ�ã���ע��������ͨ���ٿ�
	//g_pCommandList->ClearState(nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectResources::GetCurrentSwapChainRTV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * m_pSwapChain->GetCurrentBackBufferIndex();
	return cpuHandle;
}
