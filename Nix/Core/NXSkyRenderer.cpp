#include "NXSkyRenderer.h"
#include "NXGlobalDefinitions.h"
#include "NXRenderStates.h"
#include "NXScene.h"
#include "NXCubeMap.h"

NXSkyRenderer::NXSkyRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXSkyRenderer::~NXSkyRenderer()
{
}

void NXSkyRenderer::Init()
{
	SetPassName("Sky (CubeMap IBL)");

	auto pCubeMap = m_pScene->GetCubeMap();
	AddInputTex(pCubeMap->GetCubeMap());
	AddOutputRT(NXCommonRT_SSSLighting);
	SetOutputDS(NXCommonRT_DepthZ);

	SetShaderFilePath("Shader\\CubeMap.fx");
	SetInputLayout(NXGlobalInputLayout::layoutP);
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());

	SetRootParams(2, 1);
	SetRootParamCBV(0, pCubeMap->GetCBObjectParams());
	SetRootParamCBV(1, pCubeMap->GetCBDataParams());
	AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitPSO();
}
