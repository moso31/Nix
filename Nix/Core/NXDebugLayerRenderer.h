#pragma once
#include "BaseDefs/DX11.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"

struct CBufferDebugLayer
{
	Vector4 RTSize; // xy: size zw: invsize
	Vector4 LayerParam0; // x: EnableShadowMap, y: ZoomScale
};

class NXTexture2D;
class NXRenderTarget;
class NXShadowMapRenderer;
class NXDebugLayerRenderer
{
	NXDebugLayerRenderer() = default;
public:
	NXDebugLayerRenderer(NXShadowMapRenderer* m_pShadowMapRenderer);

	void Init();
	void OnResize(const Vector2& rtSize);
	void Render();

	void Release();

	NXTexture2D*	GetDebugLayerTex();

	bool	GetEnableDebugLayer()							{ return m_bEnableDebugLayer; }
	bool	GetEnableShadowMapDebugLayer()					{ return m_bEnableShadowMapDebugLayer; }
	float	GetShadowMapDebugLayerZoomScale()				{ return m_fShadowMapZoomScale; }
	void	SetEnableDebugLayer(bool value)					{ m_bEnableDebugLayer = value; }
	void	SetEnableShadowMapDebugLayer(bool value)		{ m_bEnableShadowMapDebugLayer = value; }
	void	SetShadowMapDebugLayerZoomScale(float value)	{ m_fShadowMapZoomScale = value; }

private:
	void RenderShadowMapAtlas();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXRenderTarget*						m_pRTQuad;
	
	// pass input resources
	NXShadowMapRenderer*				m_pShadowMapRenderer;

	// pass output resources
	Ntr<NXTexture2D>					m_pDebugLayerTex;

	ComPtr<ID3D11Buffer>				m_cbParams;
	CBufferDebugLayer					m_cbDataParams;

	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
};
