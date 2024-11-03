#include "BaseDefs/NixCore.h"
#include "NXGlobalDefinitions.h"

#include "NXColorMappingRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXResourceManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"
#include "NXSamplerManager.h"

NXColorMappingRenderer::NXColorMappingRenderer() :
	m_bEnablePostProcessing(true)
{
}

NXColorMappingRenderer::~NXColorMappingRenderer()
{
}

void NXColorMappingRenderer::Init()
{
	SetPassName("Color Mapping");
	SetShaderFilePath(L"Shader\\ColorMapping.fx");

	RegisterTextures(1, 1);
	SetInputTex(0, NXCommonRT_SSSLighting);
	SetOutputRT(0, NXCommonRT_PostProcessing);

	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	SetRootParams(1, 1); // b2, t0
	SetStaticRootParamCBV(0, 2, &m_cb.GetFrameGPUAddresses());

	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitPSO();
}

void NXColorMappingRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	m_cbData.param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
	m_cb.Update(m_cbData);

	NXRendererPass::Render(pCmdList);
}

void NXColorMappingRenderer::Release()
{
}
