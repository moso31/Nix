#pragma once
#include "Header.h"

class DirectResources
{
public:
	void	InitDevice();
	void	OnResize(UINT width, UINT height);
	void	Release();
	Vector2	GetViewSize();

	Vector2						GetViewPortSize();
	ID3D11ShaderResourceView*	GetOffScreenSRV()		{ return m_pOffScreenSRV.Get(); }
	ID3D11RenderTargetView*		GetOffScreenRTV()		{ return m_pOffScreenRTV.Get(); }
	ID3D11RenderTargetView*		GetRenderTargetView()	{ return m_pRenderTargetView.Get(); }
	ID3D11DepthStencilView*		GetDepthStencilView()	{ return m_pDepthStencilView.Get(); }

private:
	D3D11_VIEWPORT				m_ViewPort;
	Vector2						m_viewSize;

	ComPtr<ID3D11ShaderResourceView>	m_pOffScreenSRV;
	ComPtr<ID3D11RenderTargetView>		m_pOffScreenRTV;
	ComPtr<ID3D11RenderTargetView>		m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView>		m_pDepthStencilView;
};