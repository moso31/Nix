#include "NXDepthPeelingRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"

#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderTarget.h"

NXDepthPeelingRenderer::NXDepthPeelingRenderer(NXScene* pScene) :
	m_pScene(pScene),
	m_peelingLayerCount(11)
{
}

NXDepthPeelingRenderer::~NXDepthPeelingRenderer()
{
}

void NXDepthPeelingRenderer::Init()
{
	auto sz = g_dxResources->GetViewSize();

	m_pSceneDepth[0] = NXResourceManager::GetInstance()->CreateTexture2D("Depth Peeling Scene Depth 0", DXGI_FORMAT_R24G8_TYPELESS, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	m_pSceneDepth[0]->CreateDSV();
	m_pSceneDepth[0]->CreateSRV();

	m_pSceneDepth[1] = NXResourceManager::GetInstance()->CreateTexture2D("Depth Peeling Scene Depth 1", DXGI_FORMAT_R24G8_TYPELESS, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	m_pSceneDepth[1]->CreateDSV();
	m_pSceneDepth[1]->CreateSRV();

	m_pSceneRT.resize(m_peelingLayerCount);
	for (int i = 0; i < m_peelingLayerCount; i++)
	{
		m_pSceneRT[i] = NXResourceManager::GetInstance()->CreateTexture2D("Depth Peeling Scene RT " + std::to_string(i), DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
		m_pSceneRT[i]->CreateRTV();
		m_pSceneRT[i]->CreateSRV();
	}

	m_pCombineRTData = new NXRenderTarget();
	m_pCombineRTData->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\Scene.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Scene.fx", "PS", &m_pPixelShader);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\Scene2.fx", "VS", &m_pVertexShader2, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\Scene2.fx", "PS", &m_pPixelShader2);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DepthPeelingCombine.fx", "VS", &m_pVertexShaderCombine, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayoutCombine);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DepthPeelingCombine.fx", "PS", &m_pPixelShaderCombine);

	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pRasterizerState = NXRasterizerState<>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA>::Create();
	m_pBlendStateOpaque = NXBlendState<>::Create();

	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	m_pSamplerLinearClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
	m_pSamplerPointClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
}

void NXDepthPeelingRenderer::Render()
{
	g_pUDA->BeginEvent(L"Depth peeling rendering");

	ID3D11DepthStencilView* pDSVSceneDepth[2] = { m_pSceneDepth[0]->GetDSV(), m_pSceneDepth[1]->GetDSV() };
	ID3D11ShaderResourceView* pSRVSceneDepth[2] = { m_pSceneDepth[0]->GetSRV(), m_pSceneDepth[1]->GetSRV() };
	g_pContext->ClearDepthStencilView(pDSVSceneDepth[0], D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->ClearDepthStencilView(pDSVSceneDepth[1], D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendStateOpaque.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	for (int i = 0; i < m_peelingLayerCount; i++)
	{
		g_pUDA->BeginEvent(L"Layer");

		// 前面采用不透明渲染
		// 最后一层强制半透渲染（总比看不到要好）
		if (i == m_peelingLayerCount - 1)
		{
			g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
		}
		else
		{
			g_pContext->OMSetBlendState(m_pBlendStateOpaque.Get(), nullptr, 0xffffffff);
		}

		if (i == 0)
		{
			g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
			g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
		}
		else
		{
			g_pContext->VSSetShader(m_pVertexShader2.Get(), nullptr, 0);
			g_pContext->PSSetShader(m_pPixelShader2.Get(), nullptr, 0);
		}

		g_pContext->IASetInputLayout(m_pInputLayout.Get());

		g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());
		g_pContext->PSSetSamplers(1, 1, m_pSamplerLinearClamp.GetAddressOf());
		g_pContext->PSSetSamplers(2, 1, m_pSamplerPointClamp.GetAddressOf());

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

		//auto pShadowMapConstantBufferTransform = m_pPassShadowMap->GetConstantBufferTransform();
		//g_pContext->PSSetConstantBuffers(4, 1, &pShadowMapConstantBufferTransform);

		g_pContext->ClearDepthStencilView(pDSVSceneDepth[i % 2], D3D11_CLEAR_DEPTH, 1.0f, 0);

		auto pMainScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene);
		auto pDepthZ = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ);
		g_pContext->CopyResource(m_pSceneRT[i]->GetTex(), pMainScene->GetTex());
		g_pContext->CopyResource(m_pSceneDepth[i % 2]->GetTex(), pDepthZ->GetTex());

		auto pRTVScene = m_pSceneRT[i]->GetRTV();
		//g_pContext->ClearRenderTargetView(pRTVScene, Colors::Black);
		g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth[i % 2]);

		g_pContext->PSSetShaderResources(11, 1, &pSRVSceneDepth[(i + 1) % 2]);

		RenderLayer(i, m_peelingLayerCount);

		ID3D11ShaderResourceView* const pNullSRV[1] = { nullptr };
		g_pContext->PSSetShaderResources(11, 1, pNullSRV);

		g_pUDA->EndEvent();
	}

	// Combine Layer
	g_pUDA->BeginEvent(L"Combine");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	g_pContext->VSSetShader(m_pVertexShaderCombine.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderCombine.Get(), nullptr, 0);

	g_pContext->IASetInputLayout(m_pInputLayoutCombine.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerPointClamp.GetAddressOf());

	auto pRTVMainScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, nullptr);

	for (int i = 0; i < m_peelingLayerCount; i++)
	{
		auto pSRVScene = m_pSceneRT[i]->GetSRV();
		g_pContext->PSSetShaderResources(i, 1, &pSRVScene);
	}

	m_pCombineRTData->Render();

	ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
	g_pContext->PSSetShaderResources(0, m_peelingLayerCount, pNullSRV);

	g_pUDA->EndEvent();

	g_pUDA->EndEvent();
}

void NXDepthPeelingRenderer::Release()
{
	for(auto pRT : m_pSceneRT) SafeDelete(pRT);
	SafeDelete(m_pSceneDepth[0]);
	SafeDelete(m_pSceneDepth[1]);

	SafeDelete(m_pCombineRTData);
}

void NXDepthPeelingRenderer::RenderLayer(UINT layerIndex, UINT layerCount)
{
	// 2022.4.14 只渲染 Transparent 物体
	for (auto pMat : m_pScene->GetMaterials())
	{
		if (pMat->GetType() == NXMaterialType::PBR_TRANSLUCENT)
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
}
