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
	
	ID3D11Texture2D*			GetTexMainScene()		{ return m_pTexMainScene.Get(); }
	ID3D11ShaderResourceView*	GetSRVMainScene()		{ return m_pSRVMainScene.Get(); }
	ID3D11RenderTargetView*		GetRTVMainScene()		{ return m_pRTVMainScene.Get(); }

	ID3D11RenderTargetView*		GetRTVFinalQuad()		{ return m_pRTVFinalQuad.Get(); }

	// û���������Դ������ȷ���������������MainScene/FinalQuad����
	// ��Ϊ����ͨ�õģ��ڻ���Scene�ͻ���RTʱ����ʹ�ô�m_pXXXDepthStencil��Դ��¼���ֵ��
	ID3D11Texture2D*			GetTexDepthStencil()	{ return m_pTexDepthStencil.Get(); }
	ID3D11ShaderResourceView*	GetSRVDepthStencil()	{ return m_pSRVDepthStencil.Get(); }
	ID3D11DepthStencilView*		GetDSVDepthStencil()	{ return m_pDSVDepthStencil.Get(); }


private:
	D3D11_VIEWPORT				m_viewPort;
	Vector2						m_viewSize;

	ComPtr<ID3D11Texture2D>				m_pTexMainScene;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVMainScene;
	ComPtr<ID3D11RenderTargetView>		m_pRTVMainScene;

	ComPtr<ID3D11RenderTargetView>		m_pRTVFinalQuad;

	ComPtr<ID3D11Texture2D>				m_pTexDepthStencil;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVDepthStencil;
	ComPtr<ID3D11DepthStencilView>		m_pDSVDepthStencil;
};