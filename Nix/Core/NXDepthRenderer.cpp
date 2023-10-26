#include "NXDepthRenderer.h"
#include "NXRenderTarget.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXSamplerStates.h"
#include "NXTexture.h"

NXDepthRenderer::NXDepthRenderer()
{
}

NXDepthRenderer::~NXDepthRenderer()
{
}

void NXDepthRenderer::Init()
{
	m_pResultRT = new NXRenderTarget();
	m_pResultRT->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\Depth.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Depth.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<false, false, D3D11_COMPARISON_LESS, true, 0xFF, 0xFF,
		D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
}

void NXDepthRenderer::Render()
{
	g_pUDA->BeginEvent(L"Depth Copy");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0x01);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	ID3D11ShaderResourceView* pSRVDepthInput = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetSRV();
	ID3D11RenderTargetView* pSRVDepthOutput = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ_R32)->GetRTV();

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->OMSetRenderTargets(1, &pSRVDepthOutput, nullptr);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Point, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(0, 1, &pSampler);

	g_pContext->PSSetShaderResources(0, 1, &pSRVDepthInput);

	m_pResultRT->Render();

	ID3D11ShaderResourceView* const pNullSRV[1] = { nullptr };
	g_pContext->PSSetShaderResources(0, 1, pNullSRV);

	g_pUDA->EndEvent();
}

void NXDepthRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
