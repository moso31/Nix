#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "ShaderStructures.h"
#include "NXConstantBuffer.h"
#include "NXGraphicPass.h"

struct CBufferShadowMapObject
{
	Matrix view;
	Matrix projection;
};

class NXScene;
class NXRenderableObject;
class NXTexture2DArray;
class NXPBRDistantLight;
class NXShadowMapRenderer : public NXGraphicPass
{
private:
	explicit NXShadowMapRenderer() = default;
public:
	NXShadowMapRenderer(NXScene* pScene);
	virtual ~NXShadowMapRenderer();

	virtual void SetupInternal() override;
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;
	void RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, NXRenderableObject* pRenderableObject);

	void Release();

	UINT	GetCascadeCount()						{ return m_cascadeCount; }
	int 	GetDepthBias()							{ return m_depthBias; }
	float	GetShadowDistance()						{ return m_shadowDistance; }
	float	GetCascadeExponentScale()				{ return m_cascadeExponentScale; }
	float	GetCascadeTransitionScale()				{ return m_cascadeTransitionScale; }

	void	SetCascadeCount(UINT value);
	void	SetDepthBias(int value);
	void	SetShadowDistance(float value);
	void	SetCascadeTransitionScale(float value);
	void	SetCascadeExponentScale(float value)	{ m_cascadeExponentScale = value; }

private:
	void RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, NXPBRDistantLight* pDirLight);

private:
	NXScene*							m_pScene;

	CBufferShadowMapObject m_cbDataCSMViewProj[8];
	NXConstantBuffer<CBufferShadowMapObject> m_cbCSMViewProj[8];

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
