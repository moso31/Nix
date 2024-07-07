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
	AddInputTex(NXCommonRT_PostProcessing);
	AddInputTex(m_pShadowMapRenderer->GetShadowMapDepthTex());
	AddOutputRT(NXCommonRT_DebugLayer);

	SetShaderFilePath(L"Shader\\DebugLayer.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	m_cbParams.CreateFrameBuffers(NXCBufferAllocator, NXDescriptorAllocator);

	SetRootParams(1, 2); // b2, t0~t1
	SetRootParamCBV(0, 2, m_cbParams.GetGPUHandleArray());
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s0

	InitPSO();
}

void NXDebugLayerRenderer::OnResize(const Vector2& rtSize)
{
	CBufferDebugLayer cbData = {
		Vector4(rtSize.x, rtSize.y, 1.0f / rtSize.x, 1.0f / rtSize.y),
		Vector4(1.0f, 0.0f, 0.0f, 0.0f)
	};
	m_cbParams.Set(cbData);

	NXRendererPass::OnResize();
}

void NXDebugLayerRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bEnableDebugLayer)
		return;

	// Update LayerParams
	m_cbParams.Get().LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
	m_cbParams.Get().LayerParam0.y = m_fShadowMapZoomScale;
	m_cbParams.UpdateBuffer();

	NXRendererPass::Render(pCmdList);
}
