#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
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

	Ntr<NXTexture2DArray> GetShadowMapDepthTex();

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
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12PipelineState>			m_pPSO;
	ComPtr<ID3D12RootSignature>			m_pRootSig;

	NXScene*							m_pScene;

	// 2022.5.10 阴影贴图（目前仅用于平行光）
	Ntr<NXTexture2DArray>				m_pShadowMapDepth;

	MultiFrame<CommittedResourceData<CBufferShadowMapObject>>	m_cbShadowMapObject;

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
