#include "NXGBufferRenderer.h"
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
	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<>::Create();
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

	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	for (int i = 0; i < 4; i++) g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);

	g_pContext->OMSetRenderTargets(4, ppRTVs, pDepthZ->GetDSV());

	// 2023.4.10 过渡材质(EasyMat)
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		auto pEasyMat = pMat->IsEasyMat();
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
	}

	// 2023.6.14 RAMTest材质(TestMat)
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		auto pTestMat = pMat->IsTestMat();
		if (pTestMat)
		{
			pTestMat->Render();
			for (auto pSubMesh : pTestMat->GetRefSubMeshes())
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

	// 2023.4.10 自定义材质
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		auto pCustomMat = pMat->IsCustomMat();
		if (pCustomMat)
		{
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
	}

	g_pUDA->EndEvent();
}

void NXGBufferRenderer::Release()
{
}
