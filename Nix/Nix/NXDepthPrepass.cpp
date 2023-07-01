#include "NXDepthPrepass.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXScene.h"
#include "NXPrefab.h"

NXDepthPrepass::NXDepthPrepass(NXScene* pScene) :
	m_pInputLayout(nullptr),
	m_pScene(pScene)
{
}

NXDepthPrepass::~NXDepthPrepass()
{
}

void NXDepthPrepass::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DepthPrepass.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DepthPrepass.fx", "PS", &m_pPixelShader);
}

void NXDepthPrepass::OnResize(const Vector2& rtSize)
{
}

void NXDepthPrepass::Render()
{
	g_pUDA->BeginEvent(L"Depth Prepass");

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);

	ID3D11RenderTargetView* ppRTVs[2] = {
		pGBufferRTA->GetRTV(),
		pGBufferRTB->GetRTV(),
	};

	g_pContext->OMSetRenderTargets(2, ppRTVs, pDepthZ->GetDSV());
	g_pContext->ClearRenderTargetView(ppRTVs[0], Colors::Black);
	g_pContext->ClearRenderTargetView(ppRTVs[1], Colors::Black);
	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	g_pContext->PSSetSamplers(0, 1, &pSampler);

	// »æÖÆ DepthPrepass
	// 2023.6.4: ÔİÊ±Í£ÓÃ
	for (auto pRenderObj : m_pScene->GetRenderableObjects())
	{
	}

	ID3D11RenderTargetView* nullViews[2] = { nullptr, nullptr };
	g_pContext->OMSetRenderTargets(2, nullViews, nullptr);

	g_pUDA->EndEvent();
}
