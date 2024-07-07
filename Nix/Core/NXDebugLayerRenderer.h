#pragma once
#include "NXRendererPass.h"
#include "NXBuffer.h"

struct CBufferDebugLayer
{
	Vector4 RTSize; // xy: size zw: invsize
	Vector4 LayerParam0; // x: EnableShadowMap, y: ZoomScale
};

class NXShadowMapRenderer;
class NXDebugLayerRenderer : public NXRendererPass
{
	NXDebugLayerRenderer() = default;
public:
	NXDebugLayerRenderer(NXShadowMapRenderer* m_pShadowMapRenderer);

	void Init();
	void OnResize(const Vector2& rtSize);
	void Render(ID3D12GraphicsCommandList* pCmdList);

	bool	GetEnableDebugLayer()							{ return m_bEnableDebugLayer; }
	bool	GetEnableShadowMapDebugLayer()					{ return m_bEnableShadowMapDebugLayer; }
	float	GetShadowMapDebugLayerZoomScale()				{ return m_fShadowMapZoomScale; }
	void	SetEnableDebugLayer(bool value)					{ m_bEnableDebugLayer = value; }
	void	SetEnableShadowMapDebugLayer(bool value)		{ m_bEnableShadowMapDebugLayer = value; }
	void	SetShadowMapDebugLayerZoomScale(float value)	{ m_fShadowMapZoomScale = value; }

private:
	NXShadowMapRenderer*				m_pShadowMapRenderer;
	NXBuffer<CBufferDebugLayer> 		m_cbParams;

	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
};
