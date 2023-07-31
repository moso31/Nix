#include "NXColorMappingRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXRenderTarget.h"
#include "NXTexture.h"

NXColorMappingRenderer::NXColorMappingRenderer() :
	m_bEnablePostProcessing(true),
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

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferColorMapping);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbParams));
}

void NXColorMappingRenderer::Render()
{
	g_pUDA->BeginEvent(L"Post Processing");

	m_cbDataParams.param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;

	g_pUDA->BeginEvent(L"Color Mapping");

	ID3D11ShaderResourceView* pSRVMainScene = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_MainScene)->GetSRV();
	ID3D11RenderTargetView* pRTVPostProcessing = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing)->GetRTV();

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	g_pContext->OMSetRenderTargets(1, &pRTVPostProcessing, nullptr);

	g_pContext->UpdateSubresource(m_cbParams.Get(), 0, nullptr, &m_cbDataParams, 0, 0);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->PSSetConstantBuffers(1, 1, m_cbParams.GetAddressOf());

	g_pContext->PSSetShaderResources(0, 1, &pSRVMainScene);

	m_pFinalRT->Render();

	g_pUDA->EndEvent();

	g_pUDA->EndEvent();
}

void NXColorMappingRenderer::Release()
{
	SafeRelease(m_pFinalRT);
}
