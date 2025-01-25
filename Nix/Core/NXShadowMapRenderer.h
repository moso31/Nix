#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "ShaderStructures.h"
#include "NXConstantBuffer.h"
#include "NXRendererPass.h"

struct CBufferShadowMapObject
{
	Matrix view;
	Matrix projection;
};

class NXScene;
class NXRenderableObject;
class NXTexture2DArray;
class NXPBRDistantLight;
class NXShadowMapRenderer : public NXRendererPass
{
private:
	explicit NXShadowMapRenderer() = default;
public:
	NXShadowMapRenderer(NXScene* pScene);
	virtual ~NXShadowMapRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);
	void RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, NXRenderableObject* pRenderableObject);

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
	void RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, NXPBRDistantLight* pDirLight);

private:
	NXScene*							m_pScene;

	// 2022.5.10 ��Ӱ��ͼ��Ŀǰ������ƽ�й⣩
	Ntr<NXTexture2DArray>				m_pShadowMapDepth;

	CBufferShadowMapObject m_cbDataCSMViewProj[8];
	NXConstantBuffer<CBufferShadowMapObject> m_cbCSMViewProj[8];

	// cascade ��������
	UINT m_cascadeCount;

	// ��Ӱ��ͼ RT Size
	UINT m_shadowMapRTSize;

	// ��Ӱ����
	float m_shadowDistance;

	// cascade ����ָ����ֵԽ�� CSM �����ľ���Խ�ߡ�
	float m_cascadeExponentScale;

	// ������ ����cascade֮�� ƽ������
	// ��ֵԽ�� ����Խƽ����
	float m_cascadeTransitionScale;

	int m_depthBias;

public:
	bool m_test_transition;
};
