#pragma once
#include "BaseDefs/DX12.h"

class DirectResources
{
public:
	void	InitDevice();
	void	InitCommandLists();

	void	OnResize(UINT width, UINT height);
	void	PrepareToRenderGUI();
	void	Release();

	D3D12_CPU_DESCRIPTOR_HANDLE	GetCurrentSwapChainRTV();

private:
	ComPtr<IDXGIFactory7>		m_pDXGIFactory;
	ComPtr<IDXGISwapChain4>		m_pSwapChain;
	ComPtr<ID3D12Fence1>		m_pFence;

	ComPtr<ID3D12Resource>		m_pSwapChainRT[2];
	ComPtr<ID3D12Resource>		m_pDepthStencilBuffer;

	// 2023.12.10 目前规划主渲染管线设计的描述符堆将被统一管理，但DXResource较特殊，暂时先使用独立的描述符堆
	ComPtr<ID3D12DescriptorHeap>	m_pRTVHeap;
};