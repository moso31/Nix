#include "NXForwardRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "RenderStates.h"

#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

NXForwardRenderer::NXForwardRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXForwardRenderer::~NXForwardRenderer()
{
}

void NXForwardRenderer::Init()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\Scene.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Scene.fx", "PS", &m_pPixelShader);
}

void NXForwardRenderer::Render(ID3D11ShaderResourceView* pSRVSSAO)
{
	g_pUDA->BeginEvent(L"Forward rendering");

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();
	auto pDSVDepthStencil = g_dxResources->GetDSVDepthStencil();
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
	g_pContext->ClearRenderTargetView(pRTVMainScene, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());
	g_pContext->PSSetSamplers(1, 1, RenderStates::SamplerLinearClamp.GetAddressOf());

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	auto pCbLights = m_pScene->GetConstantBufferLights();
	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCbLights)
		g_pContext->PSSetConstantBuffers(2, 1, &pCbLights);

	if (pCubeMap)
	{
		auto pCubeMapSRV = pCubeMap->GetSRVCubeMap();
		auto pIrradianceMapSRV = pCubeMap->GetSRVIrradianceMap();
		auto pPreFilterMapSRV = pCubeMap->GetSRVPreFilterMap();
		auto pBRDF2DLUT = pCubeMap->GetSRVBRDF2DLUT();
		g_pContext->PSSetShaderResources(0, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(8, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(9, 1, &pBRDF2DLUT);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(5, 1, &pCBCubeMapParam);
	}

	// PBR大改。阴影贴图暂时停用。
	//auto pShadowMapSRV = m_pPassShadowMap->GetSRV();
	//g_pContext->PSSetShaderResources(10, 1, &pShadowMapSRV);

	if (pSRVSSAO)
	{
		g_pContext->PSSetShaderResources(11, 1, &pSRVSSAO);
	}

	//auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
	//g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);
	
	for (auto pMat : m_pScene->GetMaterials())
	{
		if (pMat->IsPBRType())
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
					pSubMesh->UpdateViewParams();
					g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

					pSubMesh->Update();
					pSubMesh->Render();
				}
			}
		}
	}

	g_pUDA->EndEvent();
}
