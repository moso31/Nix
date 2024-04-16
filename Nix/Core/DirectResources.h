#pragma once
#include "BaseDefs/DX12.h"

class DirectResources
{
public:
	void	InitDevice();

	void	FrameBegin();
	void	OnResize(UINT width, UINT height);
	void	FrameEnd();

	void	Release();

	D3D12_CPU_DESCRIPTOR_HANDLE	GetCurrentSwapChainRTV();

private:
	ComPtr<IDXGIFactory7>		m_pDXGIFactory;
	ComPtr<IDXGISwapChain4>		m_pSwapChain;
	ComPtr<ID3D12Fence1>		m_pFence;

	MultiFrame<ComPtr<ID3D12Resource>>				m_pSwapChainRT;

	UINT64 m_currFenceValue = 0;
	 
	ComPtr<ID3D12Resource>			m_pDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap;
};