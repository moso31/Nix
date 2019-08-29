#pragma once
#include "Header.h"

class DirectResources
{
public:
	void	InitDevice();
	void	OnResize(UINT width, UINT height);
	void	ClearDevices();

	Vector2						GetViewPortSize();
	ID3D11RenderTargetView*		GetRenderTargetView() { return m_pRenderTargetView; }
	ID3D11DepthStencilView*		GetDepthStencilView() { return m_pDepthStencilView; }

private:
	D3D11_VIEWPORT				m_ViewPort;

	ID3D11RenderTargetView1*	m_pRenderTargetView;
	ID3D11DepthStencilView*		m_pDepthStencilView;
};