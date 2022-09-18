#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderTarget.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::Init()
{
	m_pResultRT = new NXRenderTarget();
	m_pResultRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DeferredRender.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DeferredRender.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_LESS_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
	
	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	m_pSamplerLinearClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
}

void NXDeferredRenderer::Render()
{
	g_pUDA->BeginEvent(L"Deferred rendering");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());
	g_pContext->PSSetSamplers(1, 1, m_pSamplerLinearClamp.GetAddressOf());

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	auto pCbLights = m_pScene->GetConstantBufferLights();
	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCbLights)
		g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	if (pCubeMap)
	{
		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		auto pIrradianceMapSRV = pCubeMap->GetSRVIrradianceMap();
		auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
		auto pBRDF2DLUT = pCubeMap->GetSRVBRDF2DLUT();
		g_pContext->PSSetShaderResources(4, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(5, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(6, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pBRDF2DLUT);

		auto pIrradianceSHSRV = pCubeMap->GetSRVIrradianceSH();
		g_pContext->PSSetShaderResources(9, 1, &pIrradianceSHSRV);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(3, 1, &pCBCubeMapParam);
	}

	g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer1);
	NXTexture2D* pGBufferRTC = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer2);
	NXTexture2D* pGBufferRTD = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer3);

	ID3D11ShaderResourceView* ppSRVs[4] = {
		pGBufferRTA->GetSRV(),
		pGBufferRTB->GetSRV(),
		pGBufferRTC->GetSRV(),
		pGBufferRTD->GetSRV(),
	};

	g_pContext->PSSetShaderResources(0, 1, &ppSRVs[0]);
	g_pContext->PSSetShaderResources(1, 1, &ppSRVs[1]);
	g_pContext->PSSetShaderResources(2, 1, &ppSRVs[2]);
	g_pContext->PSSetShaderResources(3, 1, &ppSRVs[3]);

	auto pSRVShadowTest = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_ShadowTest)->GetSRV();
	g_pContext->PSSetShaderResources(8, 1, &pSRVShadowTest);

	m_pResultRT->Render();

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
