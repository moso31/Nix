#include "NXGBufferRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

NXGBufferRenderer::NXGBufferRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBufferRenderer::~NXGBufferRenderer()
{
}

void NXGBufferRenderer::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\GBuffer.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\GBuffer.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();

	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	m_pSamplerLinearClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
}

void NXGBufferRenderer::Render()
{
	g_pUDA->BeginEvent(L"GBuffer");
	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	NXTexture2D* pGBufferRTC = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	NXTexture2D* pGBufferRTD = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);

	ID3D11RenderTargetView* ppRTVs[4] = {
		pGBufferRTA->GetRTV(),
		pGBufferRTB->GetRTV(),
		pGBufferRTC->GetRTV(),
		pGBufferRTD->GetRTV(),
	};

	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	for (int i = 0; i < 4; i++) g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);

	g_pContext->OMSetRenderTargets(4, ppRTVs, pDepthZ->GetDSV());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());

	// 2022.4.14 Ö»äÖÈ¾ Opaque ÎïÌå
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		if (pMat->GetType() == NXMaterialType::PBR_STANDARD)
		{
			NXPBRMaterialBase* pPBRMat = static_cast<NXPBRMaterialBase*>(pMat);

			auto pSRVAlbedo = pPBRMat->GetSRVAlbedo();
			g_pContext->PSSetShaderResources(1, 1, &pSRVAlbedo);

			auto pSRVNormal = pPBRMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(2, 1, &pSRVNormal);

			auto pSRVMetallic = pPBRMat->GetSRVMetallic();
			g_pContext->PSSetShaderResources(3, 1, &pSRVMetallic);

			auto pSRVRoughness = pPBRMat->GetSRVRoughness();
			g_pContext->PSSetShaderResources(4, 1, &pSRVRoughness);

			auto pSRVAO = pPBRMat->GetSRVAO();
			g_pContext->PSSetShaderResources(5, 1, &pSRVAO);

			auto pCBMaterial = pPBRMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(3, 1, &pCBMaterial);

			for (auto pSubMesh : pPBRMat->GetRefSubMeshes())
			{
				if (pSubMesh)
				{
					bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
					if (bIsVisible)
					{
						pSubMesh->UpdateViewParams();
						g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

						pSubMesh->Update();
						pSubMesh->Render();
					}
				}
			}
		}
	}

	g_pUDA->EndEvent();
}

void NXGBufferRenderer::Release()
{
}
