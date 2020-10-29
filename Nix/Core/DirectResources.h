#pragma once
#include "Header.h"

class DirectResources
{
public:
	void	InitDevice();
	void	OnResize(UINT width, UINT height);
	void	ClearDevices();
	Vector2	GetViewSize();

	Vector2						GetViewPortSize();
	ID3D11ShaderResourceView*	GetOffScreenSRV()		{ return m_pOffScreenSRV; }
	ID3D11RenderTargetView*		GetOffScreenRTV()		{ return m_pOffScreenRTV; }
	ID3D11RenderTargetView*		GetRenderTargetView()	{ return m_pRenderTargetView; }
	ID3D11DepthStencilView*		GetDepthStencilView()	{ return m_pDepthStencilView; }

private:
	D3D11_VIEWPORT				m_ViewPort;
	Vector2						m_viewSize;

	ID3D11ShaderResourceView*	m_pOffScreenSRV;
	ID3D11RenderTargetView*		m_pOffScreenRTV;
	ID3D11RenderTargetView*		m_pRenderTargetView;
	ID3D11DepthStencilView*		m_pDepthStencilView;
};