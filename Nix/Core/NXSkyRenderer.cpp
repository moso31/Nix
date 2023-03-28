#include "NXSkyRenderer.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "NXRenderStates.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXTexture.h"

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
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\CubeMap.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMap.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_LESS_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();

	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
}

void NXSkyRenderer::Render()
{
	g_pUDA->BeginEvent(L"Sky (CubeMap IBL)");
	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());
		g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(1, 1, &pCBCubeMapParam);

		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);

		pCubeMap->Render();
	}

	g_pUDA->EndEvent();
}
