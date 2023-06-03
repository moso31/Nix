#pragma once
#include "Header.h"

class DirectResources
{
public:
	void	InitDevice();
	void	OnResize(UINT width, UINT height);
	void	PrepareToRenderGUI();
	void	Release();
	Vector2	GetViewSize();

	Vector2						GetViewPortSize();
	D3D11_VIEWPORT				GetViewPort()			{ return m_viewPort; }

private:
	D3D11_VIEWPORT				m_viewPort;
	Vector2						m_viewSize;

	ComPtr<ID3D11RenderTargetView>		m_pRTVSwapChainBuffer;
};