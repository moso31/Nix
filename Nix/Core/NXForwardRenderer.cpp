#include "NXForwardRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"

#include "NXBRDFlut.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

NXForwardRenderer::NXForwardRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut) :
	m_pBRDFLut(pBRDFLut),
	m_pScene(pScene)
{
}

NXForwardRenderer::~NXForwardRenderer()
{
}

void NXForwardRenderer::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ForwardTranslucent.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ForwardTranslucent.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA>::Create();
}

void NXForwardRenderer::Render()
{
	g_pUDA->BeginEvent(L"Forward rendering");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
	g_pContext->PSSetSamplers(0, 1, &pSampler);
	pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(1, 1, &pSampler);

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	auto pCbLights = m_pScene->GetConstantBufferLights();
	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCbLights)
		g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	if (pCubeMap)
	{
		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
		auto pBRDF2DLUT = m_pBRDFLut->GetSRV();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(8, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(9, 1, &pBRDF2DLUT);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(5, 1, &pCBCubeMapParam);
	}

	// PBR大改。阴影贴图暂时停用。
	//auto pShadowMapSRV = m_pPassShadowMap->GetSRV();
	//g_pContext->PSSetShaderResources(10, 1, &pShadowMapSRV);

	//auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
	//g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);

	// 2022.4.14 只渲染 Transparent 物体
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		// TODO
	}

	g_pUDA->EndEvent();
}
