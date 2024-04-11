#include "DirectResources.h"
#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"

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

	NXGlobalDX::Init(pAdapter4.Get());

	hr = NXGlobalDX::GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	RECT rc;
	GetClientRect(NXGlobalWindows::hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;
	OnResize(width, height);
}

void DirectResources::FrameBegin()
{
	MultiFrameSets::swapChainIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	auto pCmdList = m_pCommandList.Current();
	pCmdList->Reset(m_pCommandAllocator.Current().Get(), nullptr);
}

void DirectResources::OnResize(UINT width, UINT height)
{
	HRESULT hr;

	if (m_pSwapChain.Get())
	{
		hr = m_pSwapChain->ResizeBuffers(MultiFrameSets_swapChainCount, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
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
		swapChainDesc.BufferCount = MultiFrameSets_swapChainCount; // n����
		swapChainDesc.OutputWindow = NXGlobalWindows::hWnd; // ���ھ��
		swapChainDesc.Windowed = true; // ����ģʽ
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ��תģʽ
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ����ȫ���л�

		ComPtr<IDXGISwapChain> pSwapChain;
		hr = m_pDXGIFactory->CreateSwapChain(NXGlobalDX::GetCmdQueue(), &swapChainDesc, &pSwapChain);
		hr = pSwapChain.As(&m_pSwapChain);

		// ������������
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = MultiFrameSets_swapChainCount; // n����
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // �ޱ�־
		rtvHeapDesc.NodeMask = 0; // ��GPU

		hr = NXGlobalDX::GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap));
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();

		// ����RTV
		for (int i = 0; i < MultiFrameSets_swapChainCount; ++i)
		{
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainRT[i]));
			NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pSwapChainRT[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += NXGlobalDX::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			std::wstring strDebugName = L"SwapChain RT " + std::to_wstring(i);
			m_pSwapChainRT[i]->SetName(strDebugName.c_str());
		}
	}
}

void DirectResources::FrameEnd()
{
	auto pCmdList = m_pCommandList.Current();
	pCmdList->Close();

	ID3D12CommandList* pCmdLists[] = { pCmdList.Get() };
	NXGlobalDX::GetCmdQueue()->ExecuteCommandLists(1, pCmdLists);

	m_pSwapChain->Present(0, 0);

	m_currFenceValue++;
	NXGlobalDX::GetCmdQueue()->Signal(m_pFence.Get(), m_currFenceValue);

	if (m_currFenceValue - m_pFence->GetCompletedValue() > MultiFrameSets_swapChainCount - 1)
	{
		//printf("%lld, %lld\n", m_currFenceValue, m_pFence->GetCompletedValue());

		HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
		m_pFence->SetEventOnCompletion(m_currFenceValue - MultiFrameSets_swapChainCount + 1, fenceEvent);

		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}
}

void DirectResources::Release()
{
	// 2023.12.11 ����ûɶ�ã���ע��������ͨ���ٿ�
	//g_pCommandList->ClearState(nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectResources::GetCurrentSwapChainRTV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += NXGlobalDX::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * MultiFrameSets::swapChainIndex;
	return cpuHandle;
}
