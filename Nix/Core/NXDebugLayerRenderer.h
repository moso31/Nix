#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "Ntr.h"
#include "NXBuffer.h"

struct CBufferDebugLayer
{
	Vector4 RTSize; // xy: size zw: invsize
	Vector4 LayerParam0; // x: EnableShadowMap, y: ZoomScale
};

class NXTexture2D;
class NXTexture2DArray;
class NXRenderTarget;
class NXShadowMapRenderer;
class NXDebugLayerRenderer
{
	NXDebugLayerRenderer() = default;
public:
	NXDebugLayerRenderer(NXShadowMapRenderer* m_pShadowMapRenderer);

	void Init(const Vector2& rtSize);
	void OnResize(const Vector2& rtSize);
	void Render();

	void Release();

	Ntr<NXTexture2D> const GetDebugLayerTex() { return m_pTexPassOut; }

	bool	GetEnableDebugLayer()							{ return m_bEnableDebugLayer; }
	bool	GetEnableShadowMapDebugLayer()					{ return m_bEnableShadowMapDebugLayer; }
	float	GetShadowMapDebugLayerZoomScale()				{ return m_fShadowMapZoomScale; }
	void	SetEnableDebugLayer(bool value)					{ m_bEnableDebugLayer = value; }
	void	SetEnableShadowMapDebugLayer(bool value)		{ m_bEnableShadowMapDebugLayer = value; }
	void	SetShadowMapDebugLayerZoomScale(float value)	{ m_fShadowMapZoomScale = value; }

private:
	Ntr<NXTexture2D>					m_pTexPassIn0; // render result.
	Ntr<NXTexture2DArray>				m_pTexPassIn1; // shadow map atlas.
	Ntr<NXTexture2D>					m_pTexPassOut; // debug layer.

	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator;

	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;
	
	// pass input resources
	NXShadowMapRenderer*				m_pShadowMapRenderer;

	NXBuffer<CBufferDebugLayer> 		m_cbParams;

	bool m_bEnableDebugLayer;
	bool m_bEnableShadowMapDebugLayer;
	float m_fShadowMapZoomScale;
};
