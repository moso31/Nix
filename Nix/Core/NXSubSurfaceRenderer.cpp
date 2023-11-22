#include "NXSubSurfaceRenderer.h"
#include "NXRenderTarget.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"
#include "NXSamplerStates.h"
#include "NXTexture.h"
#include "NXScene.h"

NXSubSurfaceRenderer::NXSubSurfaceRenderer(NXScene* pScene) :
	m_pScene(pScene)
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

	m_pDepthStencilState = NXDepthStencilState<false, false, D3D11_COMPARISON_LESS, true, 0xFF, 0xFF,
		D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
}

void NXSubSurfaceRenderer::Render()
{
	g_pUDA->BeginEvent(L"SSSSS");
	static int RenderMode = 0;
	if (RenderMode == 0) RenderSSSSS();
	g_pUDA->EndEvent();
}

void NXSubSurfaceRenderer::RenderSSSSS()
{
	auto& cbDiffuseProfile = NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile();
	g_pContext->PSSetConstantBuffers(3, 1, cbDiffuseProfile.GetAddressOf());

	auto pCBLight = m_pScene->GetConstantBufferLights();
	g_pContext->PSSetConstantBuffers(2, 1, &pCBLight);

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0x01);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	ID3D11ShaderResourceView* pSRVIrradiance = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting0)->GetSRV();
	ID3D11ShaderResourceView* pSRVSpecLighting = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting1)->GetSRV();
	ID3D11ShaderResourceView* pSRVSSSTransmit = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_Lighting2)->GetSRV();
	ID3D11ShaderResourceView* pSRVNormal = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1)->GetSRV();
	ID3D11ShaderResourceView* pSRVDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ_R32)->GetSRV();
	ID3D11ShaderResourceView* pSRVNoiseGray = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64)->GetSRV();
	ID3D11DepthStencilView* pDSVDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	ID3D11RenderTargetView* pRTVBurleySSSOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting)->GetRTV();

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->OMSetRenderTargets(1, &pRTVBurleySSSOut, pDSVDepthZ);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(0, 1, &pSampler);

	g_pContext->PSSetShaderResources(0, 1, &pSRVIrradiance);
	g_pContext->PSSetShaderResources(1, 1, &pSRVSpecLighting);
	g_pContext->PSSetShaderResources(2, 1, &pSRVSSSTransmit);
	g_pContext->PSSetShaderResources(3, 1, &pSRVNormal);
	g_pContext->PSSetShaderResources(4, 1, &pSRVDepthZ);
	g_pContext->PSSetShaderResources(5, 1, &pSRVNoiseGray);

	m_pResultRT->Render();

	ID3D11ShaderResourceView* const pNullSRV[6] = { nullptr };
	g_pContext->PSSetShaderResources(0, 6, pNullSRV);
}

void NXSubSurfaceRenderer::Release()
{
	SafeRelease(m_pResultRT);
}
