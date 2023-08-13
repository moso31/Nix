#include "NXShadowTestRenderer.h"
#include "GlobalBufferManager.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "NXResourceManager.h"
#include "NXRenderTarget.h"
#include "NXTexture.h"

NXShadowTestRenderer::~NXShadowTestRenderer()
{
}

void NXShadowTestRenderer::Init()
{
	m_pResultRT = new NXRenderTarget();
	m_pResultRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ShadowTest.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ShadowTest.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_ALWAYS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create(1000);
	m_pBlendState = NXBlendState<>::Create();
}

void NXShadowTestRenderer::Render(NXTexture2DArray* pShadowMapDepthTex)
{
	g_pUDA->BeginEvent(L"Shadow Test");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pSRVSceneDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetSRV();
	auto pSRVShadowDepth = pShadowMapDepthTex->GetSRV();
	auto pRTVShadowTest = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_ShadowTest)->GetRTV();
	g_pContext->ClearRenderTargetView(pRTVShadowTest, Colors::Black);
	g_pContext->OMSetRenderTargets(1, &pRTVShadowTest, nullptr);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->PSSetShaderResources(0, 1, &pSRVShadowDepth);
	g_pContext->PSSetShaderResources(1, 1, &pSRVSceneDepth);
	g_pContext->VSSetConstantBuffers(2, 1, NXGlobalBufferManager::m_cbShadowTest.GetAddressOf());
	g_pContext->PSSetConstantBuffers(2, 1, NXGlobalBufferManager::m_cbShadowTest.GetAddressOf());

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Point, NXSamplerAddressMode::Border);
	g_pContext->PSSetSamplers(0, 1, &pSampler);

	m_pResultRT->Render();

	ID3D11ShaderResourceView* const pNullSRV[3] = { nullptr };
	g_pContext->PSSetShaderResources(0, 3, pNullSRV);

	g_pUDA->EndEvent();
}

void NXShadowTestRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
