#pragma once
#include "BaseDefs/DX12.h"

struct NXSwapChainBuffer
{
	void SetResourceStates(D3D12_RESOURCE_STATES state) 
	{
		resourceState = state;
	}

	ComPtr<ID3D12Resource> pBuffer;
	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET; // 交换链的 RT 初始化后都是 D3D12_RESOURCE_STATE_RENDER_TARGET
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
};

class DirectResources
{
public:
	DirectResources();

	void	InitDevice();

	void	FrameBegin();
	void	OnResize(UINT width, UINT height);
	void	FrameEnd();

	void	Flush();
	void	Release();

	const NXSwapChainBuffer& GetCurrentSwapChain() { return m_pSwapChainBuffer.Current(); }

private:
	void	CreateSwapChain(UINT width, UINT height);
	void	CreateSwapChainRTVHeap();
	void	RemoveSwapChainRTVHeap();

private:
	ComPtr<IDXGIFactory7>		m_pDXGIFactory;
	ComPtr<IDXGISwapChain4>		m_pSwapChain;

	MultiFrame<NXSwapChainBuffer>	m_pSwapChainBuffer;
	DXGI_FORMAT m_pSwapChainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	 
	ComPtr<ID3D12Resource>			m_pDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap;

	HANDLE m_fenceEvent;
	std::vector<uint64_t> m_fenceValues;
};