#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXGlobalBuffers.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXSubMeshGeometryEditor.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::SetupInternal()
{
	SetPassName("Deferred Rendering");

	SetShaderFilePath("Shader\\DeferredRender.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());

	// t0~t8, s0~s1, b0~b4.
	SetRootParams(5, 9); // param 0~4 = b0~b4, param 5 = t0~t8
	SetStaticRootParamCBV(0, &g_cbObject.GetFrameGPUAddresses());
	SetStaticRootParamCBV(1, &g_cbCamera.GetFrameGPUAddresses());
	SetStaticRootParamCBV(2, &m_pScene->GetConstantBufferLights());
	SetStaticRootParamCBV(3, &m_pScene->GetCubeMap()->GetCBDataParams());
	SetStaticRootParamCBV(4, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

	AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	InitPSO();
}
