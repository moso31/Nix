#include "NXSubSurfaceRenderer.h"
#include "NXRenderTarget.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXSamplerStates.h"
#include "NXTexture.h"

NXSubSurfaceRenderer::NXSubSurfaceRenderer()
{
}

NXSubSurfaceRenderer::~NXSubSurfaceRenderer()
{
}

void NXSubSurfaceRenderer::Init()
{
	m_pResultRT = new NXRenderTarget();
	m_pResultRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\SSSSSRenderer.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\SSSSSRenderer.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_LESS_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
}

void NXSubSurfaceRenderer::Render()
{
	static int RenderMode = 0;
	if (RenderMode == 0) RenderSSSSS();
}

void NXSubSurfaceRenderer::RenderSSSSS()
{
	g_pUDA->BeginEvent(L"SSSSS");

	ID3D11ShaderResourceView* pSRVIrradiance = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0)->GetSRV();
	ID3D11ShaderResourceView* pSRVSpecLighting = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting1)->GetSRV();
	ID3D11RenderTargetView* pRTVOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting)->GetRTV();

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->OMSetRenderTargets(1, &pRTVOut, nullptr);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(0, 1, &pSampler);

	g_pContext->PSSetShaderResources(0, 1, &pSRVIrradiance);
	g_pContext->PSSetShaderResources(1, 1, &pSRVSpecLighting);

	m_pResultRT->Render();

	g_pUDA->EndEvent();
}

void NXSubSurfaceRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
