#pragma once
#include "BaseDefs/DX11.h"
#include "BaseDefs/Math.h"
#include "ShaderStructures.h"

struct CBufferShadowMapObject
{
	Matrix world;
	Matrix view;
	Matrix projection;
};

class NXScene;
class NXRenderableObject;
class NXTexture2DArray;
class NXPBRDistantLight;
class NXShadowMapRenderer
{
private:
	explicit NXShadowMapRenderer() = default;
public:
	NXShadowMapRenderer(NXScene* pScene);
	~NXShadowMapRenderer();

	void Init();
	void Render();
	void RenderSingleObject(NXRenderableObject* pRenderableObject);

	void Release();

	NXTexture2DArray* GetShadowMapDepthTex() { return m_pShadowMapDepth.Ptr(); }

	UINT	GetCascadeCount()						{ return m_cascadeCount; }
	int 	GetDepthBias()							{ return m_depthBias; }
	UINT	GetShadowMapRTSize()					{ return m_shadowMapRTSize; }
	float	GetShadowDistance()						{ return m_shadowDistance; }
	float	GetCascadeExponentScale()				{ return m_cascadeExponentScale; }
	float	GetCascadeTransitionScale()				{ return m_cascadeTransitionScale; }

	void	SetCascadeCount(UINT value);
	void	SetDepthBias(int value);
	void	SetShadowDistance(float value);
	void	SetCascadeTransitionScale(float value);
	void	SetShadowMapRTSize(UINT value)			{ m_shadowMapRTSize = value; }
	void	SetCascadeExponentScale(float value)	{ m_cascadeExponentScale = value; }

private:
	void UpdateShadowMapBuffer(NXPBRDistantLight* pDirLight);

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXScene*							m_pScene;

	// 2022.5.10 阴影贴图（目前仅用于平行光）
	Ntr<NXTexture2DArray>				m_pShadowMapDepth;

	ComPtr<ID3D11Buffer>				m_cbShadowMapObject;
	CBufferShadowMapObject				m_cbDataShadowMapObject;

	// cascade 级联数量
	UINT m_cascadeCount;

	// 阴影贴图 RT Size
	UINT m_shadowMapRTSize;

	// 阴影距离
	float m_shadowDistance;

	// cascade 缩放指数，值越高 CSM 近处的精度越高。
	float m_cascadeExponentScale;

	// 用于在 两级cascade之间 平滑过渡
	// 该值越大 过渡越平滑。
	float m_cascadeTransitionScale;

	int m_depthBias;

public:
	bool m_test_transition;
};
