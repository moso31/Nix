#include "NXGBufferRenderer.h"
#include "Global.h"
#include "Ntr.h"

#include "ShaderComplier.h"
#include "NXHLSLGenerator.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

#include "NXPBRMaterial.h"

NXGBufferRenderer::NXGBufferRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBufferRenderer::~NXGBufferRenderer()
{
}

void NXGBufferRenderer::Init()
{
	m_pDepthStencilState = NXDepthStencilState<true, true, D3D11_COMPARISON_LESS, true, 0xFF, 0xFF, 
		D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
}

void NXGBufferRenderer::Render()
{
	g_pUDA->BeginEvent(L"GBuffer");
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto& pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	auto& pGBufferRTA = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	auto& pGBufferRTB = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	auto& pGBufferRTC = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	auto& pGBufferRTD = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);

	ID3D11RenderTargetView* ppRTVs[4] = {
		pGBufferRTA->GetRTV(),
		pGBufferRTB->GetRTV(),
		pGBufferRTC->GetRTV(),
		pGBufferRTD->GetRTV(),
	};

	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	for (int i = 0; i < 4; i++) g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);

	g_pContext->OMSetRenderTargets(4, ppRTVs, pDepthZ->GetDSV());

	auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
	auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
	for (auto pMat : pMaterialsArray)
	{
		auto pEasyMat = pMat ? pMat->IsEasyMat() : nullptr;
		auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
		g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);

		if (pEasyMat)
		{
			pEasyMat->Render();
			for (auto pSubMesh : pEasyMat->GetRefSubMeshes())
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
		else if (pCustomMat)
		{
			if (pCustomMat->GetCompileSuccess())
			{
				// 3S材质需要写模板缓存
				if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
					g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0x01);

				pCustomMat->Render();

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
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
			else
			{
				pErrorMat->Render();

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
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
	}

	g_pUDA->EndEvent();
}

void NXGBufferRenderer::Release()
{
}
