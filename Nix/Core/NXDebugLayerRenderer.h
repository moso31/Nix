#pragma once
#include "NXRendererPass.h"
#include "NXConstantBuffer.h"

struct CBufferDebugLayer
{
	Vector4 LayerParam0; // x: EnableShadowMap, y: ZoomScale
};

class NXShadowMapRenderer;
class NXDebugLayerRenderer : public NXRendererPass
{
public:
	NXDebugLayerRenderer();

	virtual void SetupInternal() override;
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;

	void	SetEnableDebugLayer(bool value)					{ m_bEnableDebugLayer = value; }
	bool	GetEnableShadowMapDebugLayer()					{ return m_bEnableShadowMapDebugLayer; }
	float	GetShadowMapDebugLayerZoomScale()				{ return m_fShadowMapZoomScale; }
	void	SetEnableShadowMapDebugLayer(bool value)		{ m_bEnableShadowMapDebugLayer = value; }
	void	SetShadowMapDebugLayerZoomScale(float value)	{ m_fShadowMapZoomScale = value; }

private:
	CBufferDebugLayer						m_cbData;
	NXConstantBuffer<CBufferDebugLayer>		m_cb;

	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
};
