#include "NXDebugLayerRenderer.h"
#include "NXShadowMapRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXGlobalDefinitions.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"

NXDebugLayerRenderer::NXDebugLayerRenderer(NXShadowMapRenderer* pShadowMapRenderer) :
	m_pShadowMapRenderer(pShadowMapRenderer),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
}

void NXDebugLayerRenderer::Init()
{
	SetPassName("Debug Layer");
	RegisterTextures(2, 1);
	SetInputTex(0, NXCommonRT_PostProcessing);
	SetInputTex(1, m_pShadowMapRenderer->GetShadowMapDepthTex());
	SetOutputRT(0, NXCommonRT_DebugLayer);

	SetShaderFilePath(L"Shader\\DebugLayer.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	SetRootParams(1, 2); // b2, t0~t1
	SetStaticRootParamCBV(0, 2, &m_cb.GetFrameGPUAddresses());
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s0

	InitPSO();
}

void NXDebugLayerRenderer::OnResize(const Vector2& rtSize)
{
	m_cbData = {
		Vector4(rtSize.x, rtSize.y, 1.0f / rtSize.x, 1.0f / rtSize.y),
		Vector4(1.0f, 0.0f, 0.0f, 0.0f)
	};

	NXRendererPass::OnResize();
}

void NXDebugLayerRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bEnableDebugLayer)
		return;

	m_cbData.LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
	m_cbData.LayerParam0.y = m_fShadowMapZoomScale;
	m_cb.Update(m_cbData);

	NXRendererPass::Render(pCmdList);
}
