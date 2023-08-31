#pragma once
#include "BaseDefs/DX11.h"

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