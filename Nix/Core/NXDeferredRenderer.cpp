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

void NXDeferredRenderer::Render()
{
	g_pUDA->BeginEvent(L"Deferred rendering");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

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

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	NXTexture2D* pGBufferRTC = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	NXTexture2D* pGBufferRTD = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);

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

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
