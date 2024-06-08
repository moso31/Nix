#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXBRDFlut.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXSubMeshGeometryEditor.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut) :
	m_pBRDFLut(pBRDFLut),
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::Init()
{
	AddInputTex(NXCommonRT_GBuffer0);
	AddInputTex(NXCommonRT_GBuffer1);
	AddInputTex(NXCommonRT_GBuffer2);
	AddInputTex(NXCommonRT_GBuffer3);
	AddInputTex(NXCommonRT_DepthZ);
	AddInputTex(m_pScene->GetCubeMap()->GetCubeMap());
	AddInputTex(m_pScene->GetCubeMap()->GetPreFilterMap());
	AddInputTex(m_pBRDFLut->GetTex());
	AddInputTex(NXCommonRT_ShadowTest);

	AddOutputRT(NXCommonRT_Lighting0);
	AddOutputRT(NXCommonRT_Lighting1);
	AddOutputRT(NXCommonRT_Lighting2);
	AddOutputRT(NXCommonRT_SSSLighting);

	SetShaderFilePath("Shader\\DeferredRender.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());

	// t0~t8, s0~s1, b0~b4.
	SetRootParams(5, 9); // param 0~4 = b0~b4, param 5 = t0~t8
	SetRootParamCBV(0, NXGlobalBuffer::cbObject.GetGPUHandle());
	SetRootParamCBV(1, NXGlobalBuffer::cbCamera.GetGPUHandle());
	SetRootParamCBV(2, m_pScene->GetConstantBufferLights());
	SetRootParamCBV(3, m_pScene->GetCubeMap()->GetCBDataParams());
	SetRootParamCBV(4, NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());
	AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	InitPSO();
}

void NXDeferredRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "Deferred Rendering");
	NXRendererPass::RenderBegin(pCmdList);
	NXRendererPass::RenderEnd(pCmdList);
	NX12Util::EndEvent(pCmdList);
}

void NXDeferredRenderer::Release()
{
}
