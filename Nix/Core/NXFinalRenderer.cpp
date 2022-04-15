#include "NXFinalRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderTarget.h"

NXFinalRenderer::NXFinalRenderer() :
	m_pFinalRT(nullptr)
{
}

NXFinalRenderer::~NXFinalRenderer()
{
}

void NXFinalRenderer::Init()
{
	m_pFinalRT = new NXRenderTarget();
	m_pFinalRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\FinalQuad.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\FinalQuad.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_ALWAYS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();

	m_pSamplerLinearClamp.Swap(NXSamplerState<>::Create());
}

void NXFinalRenderer::Render()
{
	g_pUDA->BeginEvent(L"Render Target");

	ID3D11RenderTargetView* pRTVFinalQuad = g_dxResources->GetRTVFinalQuad();
	ID3D11ShaderResourceView* pSRVPostProcessing = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_PostProcessing)->GetSRV();

	g_pContext->OMSetRenderTargets(1, &pRTVFinalQuad, nullptr);
	g_pContext->ClearRenderTargetView(pRTVFinalQuad, Colors::Black);

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	g_pContext->PSSetShaderResources(0, 1, &pSRVPostProcessing);

	m_pFinalRT->Render();

	g_pUDA->EndEvent();
}

void NXFinalRenderer::Release()
{
	SafeRelease(m_pFinalRT);
}
