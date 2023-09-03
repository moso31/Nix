#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXBRDFlut.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderTarget.h"

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
	m_pResultRT = new NXRenderTarget();
	m_pResultRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DeferredRender.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DeferredRender.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_LESS_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
}

void NXDeferredRenderer::Render(bool bSSSEnable)
{
	g_pUDA->BeginEvent(L"Deferred rendering");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVDeferredLighting = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0)->GetRTV();
	auto pRTVDeferredLightingEx = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting1)->GetRTV();
	auto pRTVSSSLighting = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting)->GetRTV();

	ID3D11RenderTargetView* ppRTVs[] = {
		pRTVDeferredLighting,
		pRTVDeferredLightingEx,
		pRTVSSSLighting,
	};

	int ppRTVSize = bSSSEnable ? ARRAYSIZE(ppRTVs) : 1;
	//for (int i = 0; i < ppRTVSize ; i++) g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);
	g_pContext->OMSetRenderTargets(ppRTVSize, ppRTVs, nullptr);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

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
		g_pContext->PSSetShaderResources(5, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(6, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pBRDF2DLUT);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(3, 1, &pCBCubeMapParam);
	}

	g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

	auto& pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	auto& pGBufferRTA = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	auto& pGBufferRTB = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	auto& pGBufferRTC = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	auto& pGBufferRTD = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);

	ID3D11ShaderResourceView* ppSRVs[] = {
		pGBufferRTA->GetSRV(),
		pGBufferRTB->GetSRV(),
		pGBufferRTC->GetSRV(),
		pGBufferRTD->GetSRV(),
		pDepthZ->GetSRV(),
	};

	g_pContext->PSSetShaderResources(0, 1, &ppSRVs[0]);
	g_pContext->PSSetShaderResources(1, 1, &ppSRVs[1]);
	g_pContext->PSSetShaderResources(2, 1, &ppSRVs[2]);
	g_pContext->PSSetShaderResources(3, 1, &ppSRVs[3]);
	g_pContext->PSSetShaderResources(4, 1, &ppSRVs[4]);

	auto pSRVShadowTest = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_ShadowTest)->GetSRV();
	g_pContext->PSSetShaderResources(8, 1, &pSRVShadowTest);

	m_pResultRT->Render();

	ID3D11ShaderResourceView* const pNullSRV[1] = { nullptr };
	g_pContext->PSSetShaderResources(4, 1, pNullSRV);

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
