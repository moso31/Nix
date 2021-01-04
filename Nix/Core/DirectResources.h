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
	D3D11_VIEWPORT				GetViewPort()			{ return m_viewPort; }
	
	ID3D11Texture2D*			GetTexOffScreen()		{ return m_pTexOffScreen.Get(); }
	ID3D11ShaderResourceView*	GetSRVOffScreen()		{ return m_pSRVOffScreen.Get(); }
	ID3D11RenderTargetView*		GetRTVOffScreen()		{ return m_pRTVOffScreen.Get(); }

	ID3D11RenderTargetView*		GetRenderTargetView()	{ return m_pRenderTargetView.Get(); }
	ID3D11DepthStencilView*		GetDepthStencilView()	{ return m_pDepthStencilView.Get(); }
	ID3D11Texture2D*			GetTexDepthStencil()	{ return m_pTexDepthStencil.Get(); }


private:
	D3D11_VIEWPORT				m_viewPort;
	Vector2						m_viewSize;

	ComPtr<ID3D11Texture2D>				m_pTexOffScreen;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVOffScreen;
	ComPtr<ID3D11RenderTargetView>		m_pRTVOffScreen;

	ComPtr<ID3D11RenderTargetView>		m_pRenderTargetView;

	ComPtr<ID3D11Texture2D>				m_pTexDepthStencil;
	ComPtr<ID3D11DepthStencilView>		m_pDepthStencilView;
};