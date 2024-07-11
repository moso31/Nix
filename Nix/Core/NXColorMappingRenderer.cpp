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

	AddInputTex(NXCommonRT_SSSLighting);
	AddOutputRT(NXCommonRT_PostProcessing);
	SetShaderFilePath(L"Shader\\ColorMapping.fx");

	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	SetRootParams(1, 1); // b2, t0
	SetStaticRootParamCBV(0, 2, NXGlobalBuffer::cbCamera.GetGPUHandleArray());

	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	m_cbParams.CreateFrameBuffers(NXCBufferAllocator, NXDescriptorAllocator);

	InitPSO();
}

void NXColorMappingRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	m_cbParams.Get().param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
	m_cbParams.UpdateBuffer();

	NXRendererPass::Render(pCmdList);
}

void NXColorMappingRenderer::Release()
{
}
