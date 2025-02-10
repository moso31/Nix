#include "NXDebugLayerRenderer.h"
#include "NXShadowMapRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXGlobalBuffers.h"

NXDebugLayerRenderer::NXDebugLayerRenderer() :
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
	m_cbData = { Vector4(1.0f, 0.0f, 0.0f, 0.0f) };
}

void NXDebugLayerRenderer::SetupInternal()
{
	SetPassName("Debug Layer");

	SetShaderFilePath(L"Shader\\DebugLayer.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	SetRootParams(2, 2); // b1, b2, t0~t1
	SetStaticRootParamCBV(0, 1, &g_cbCamera.GetFrameGPUAddresses());
	SetStaticRootParamCBV(1, 2, &m_cb.GetFrameGPUAddresses());
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s0

	InitPSO();
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
