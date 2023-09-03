#include "NXDebugLayerRenderer.h"
#include "NXShadowMapRenderer.h"
#include "NXRenderTarget.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXTexture.h"

NXDebugLayerRenderer::NXDebugLayerRenderer(NXShadowMapRenderer* pShadowMapRenderer) :
	m_pShadowMapRenderer(pShadowMapRenderer),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
}

void NXDebugLayerRenderer::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DebugLayer.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DebugLayer.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<true, false, D3D11_COMPARISON_ALWAYS>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();

	m_pRTQuad = new NXRenderTarget();
	m_pRTQuad->Init();

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferDebugLayer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbParams));
}

void NXDebugLayerRenderer::OnResize(const Vector2& rtSize)
{
	m_pDebugLayerTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D_Internal("Debug Layer Out RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
	m_pDebugLayerTex->AddRTV();
	m_pDebugLayerTex->AddSRV();

	m_cbDataParams.RTSize = Vector4(rtSize.x, rtSize.y, 1.0f / rtSize.x, 1.0f / rtSize.y);
	m_cbDataParams.LayerParam0 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
}

void NXDebugLayerRenderer::Render()
{
	if (!m_bEnableDebugLayer)
		return;

	// Update LayerParams
	m_cbDataParams.LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
	m_cbDataParams.LayerParam0.y = m_fShadowMapZoomScale;

	g_pUDA->BeginEvent(L"Debug Layer");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVOutput = m_pDebugLayerTex->GetRTV();
	g_pContext->OMSetRenderTargets(1, &pRTVOutput, nullptr);

	g_pContext->UpdateSubresource(m_cbParams.Get(), 0, nullptr, &m_cbDataParams, 0, 0);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Point, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(0, 1, &pSampler);
	g_pContext->PSSetConstantBuffers(2, 1, m_cbParams.GetAddressOf());

	auto& pSceneInputTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);
	auto pSRVRenderResult = pSceneInputTex->GetSRV();
	g_pContext->PSSetShaderResources(0, 1, &pSRVRenderResult);	// 第0张Input的RT写死，一定是SceneInputTexture

	if (m_bEnableShadowMapDebugLayer)
		RenderShadowMapAtlas();

	m_pRTQuad->Render();

	g_pUDA->EndEvent();
}

void NXDebugLayerRenderer::Release()
{
	SafeRelease(m_pRTQuad);
}

void NXDebugLayerRenderer::RenderShadowMapAtlas()
{
	NXTexture2DArray* pShadowMapDepthTex = m_pShadowMapRenderer->GetShadowMapDepthTex();
	auto pSRVShadowDepth = pShadowMapDepthTex->GetSRV();
	g_pContext->PSSetShaderResources(1, 1, &pSRVShadowDepth);
}
