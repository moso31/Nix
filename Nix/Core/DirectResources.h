#pragma once
#include "Header.h"

class DirectResources
{
public:
	void	InitDevice();
	void	OnResize(UINT width, UINT height);
	void	PrepareToRenderGUI();
	void	Release();

private:
	ComPtr<ID3D11RenderTargetView>		m_pRTVSwapChainBuffer;
};