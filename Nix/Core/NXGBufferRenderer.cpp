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

NXGBufferRenderer::NXGBufferRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBufferRenderer::~NXGBufferRenderer()
{
}

void NXGBufferRenderer::Init()
{
	//NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\GBuffer.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	//NXShaderComplier::GetInstance()->CompilePS(L"Shader\\GBuffer.fx", "PS", &m_pPixelShader);

	NXShaderResourceInfoArray srInfoArray;
	std::string strShader, strShaderParam, strShaderCode;
	NXHLSLGenerator::GetInstance()->LoadShaderFromFile("Shader\\GBufferEx_Test.nsl", strShader);
	NXHLSLGenerator::GetInstance()->ConvertShaderToHLSL("Shader\\GBufferEx_Test.nsl", strShader, strShaderParam, strShaderCode, srInfoArray);
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strShaderParam, strShaderCode, strShader);

	NXShaderComplier::GetInstance()->CompileVSILByCode(strShader, "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePSByCode(strShader, "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pDepthStencilStateSSS = NXDepthStencilState<true, true, D3D11_COMPARISON_LESS, true, 0xff, 0xff, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS>::Create();
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

	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	for (int i = 0; i < 4; i++) g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);

	g_pContext->OMSetRenderTargets(4, ppRTVs, pDepthZ->GetDSV());

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());

	// 2022.4.14 渲染 Opaque 物体
	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		auto pStandardMat = pMat->IsStandardMat();
		if (pStandardMat)
		{
			auto pSRVAlbedo = pStandardMat->GetSRVAlbedo();
			g_pContext->PSSetShaderResources(1, 1, &pSRVAlbedo);

			auto pSRVNormal = pStandardMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(2, 1, &pSRVNormal);

			auto pSRVMetallic = pStandardMat->GetSRVMetallic();
			g_pContext->PSSetShaderResources(3, 1, &pSRVMetallic);

			auto pSRVRoughness = pStandardMat->GetSRVRoughness();
			g_pContext->PSSetShaderResources(4, 1, &pSRVRoughness);

			auto pSRVAO = pStandardMat->GetSRVAO();
			g_pContext->PSSetShaderResources(5, 1, &pSRVAO);

			auto pCBMaterial = pStandardMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(3, 1, &pCBMaterial);

			for (auto pSubMesh : pStandardMat->GetRefSubMeshes())
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

	// 2023.4.10 自定义材质（临时）
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

	// 2023.4.4 渲染3S材质
	g_pContext->OMSetDepthStencilState(m_pDepthStencilStateSSS.Get(), 0x25);

	for (auto pMat : NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials())
	{
		auto pSubsurfaceMat = pMat->IsSubsurfaceMat();
		if (pSubsurfaceMat)
		{
			auto pSRVAlbedo = pSubsurfaceMat->GetSRVAlbedo();
			g_pContext->PSSetShaderResources(1, 1, &pSRVAlbedo);

			auto pSRVNormal = pSubsurfaceMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(2, 1, &pSRVNormal);

			auto pSRVMetallic = pSubsurfaceMat->GetSRVMetallic();
			g_pContext->PSSetShaderResources(3, 1, &pSRVMetallic);

			auto pSRVRoughness = pSubsurfaceMat->GetSRVRoughness();
			g_pContext->PSSetShaderResources(4, 1, &pSRVRoughness);

			auto pSRVAO = pSubsurfaceMat->GetSRVAO();
			g_pContext->PSSetShaderResources(5, 1, &pSRVAO);

			auto pCBMaterial = pSubsurfaceMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(3, 1, &pCBMaterial);

			for (auto pSubMesh : pSubsurfaceMat->GetRefSubMeshes())
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
