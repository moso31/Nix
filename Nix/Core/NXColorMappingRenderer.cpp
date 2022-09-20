#include "NXColorMappingRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXRenderTarget.h"

NXColorMappingRenderer::NXColorMappingRenderer() :
	m_pFinalRT(nullptr)
{
}

NXColorMappingRenderer::~NXColorMappingRenderer()
{
}

void NXColorMappingRenderer::Init()
{
	m_pFinalRT = new NXRenderTarget();
	m_pFinalRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ColorMapping.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ColorMapping.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_ALWAYS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();

	m_pSamplerLinearClamp.Swap(NXSamplerState<>::Create());
}

void NXColorMappingRenderer::Render()
{
	g_pUDA->BeginEvent(L"Post Processing");

	g_pUDA->BeginEvent(L"Color Mapping");

	ID3D11ShaderResourceView* pSRVMainScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetSRV();
	ID3D11RenderTargetView* pRTVPostProcessing = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_PostProcessing)->GetRTV();

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	g_pContext->OMSetRenderTargets(1, &pRTVPostProcessing, nullptr);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	g_pContext->PSSetShaderResources(0, 1, &pSRVMainScene);

	m_pFinalRT->Render();

	g_pUDA->EndEvent();

	g_pUDA->EndEvent();
}

void NXColorMappingRenderer::Release()
{
	SafeRelease(m_pFinalRT);
}
