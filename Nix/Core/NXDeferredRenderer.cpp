#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

NXDeferredRenderer::NXDeferredRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXDeferredRenderer::~NXDeferredRenderer()
{
}

void NXDeferredRenderer::Init()
{
	float scale = 1.0f;
	// Create vertex buffer
	m_vertices =
	{
		// -Z
		{ Vector3(-scale, +scale, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+scale, +scale, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(+scale, -scale, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-scale, -scale, 0.0f), Vector2(0.0f, 1.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitVertexIndexBuffer();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\GBuffer.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayoutGBuffer);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\GBuffer.fx", "PS", &m_pPixelShader);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DeferredRender.fx", "VS", &m_pVertexShaderRender, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayoutRender);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DeferredRender.fx", "PS", &m_pPixelShaderRender);

	m_pDepthStencilStateGBuffer = NXDepthStencilState<>::Create();
	m_pRasterizerStateGBuffer = NXRasterizerState<>::Create();
	m_pBlendStateGBuffer = NXBlendState<>::Create();

	m_pDepthStencilStateLighting = NXDepthStencilState<true, false, D3D11_COMPARISON_LESS_EQUAL>::Create();
	m_pRasterizerStateLighting = NXRasterizerState<>::Create();
	m_pBlendStateLighting = NXBlendState<>::Create();
	
	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	m_pSamplerLinearClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
}

void NXDeferredRenderer::RenderGBuffer()
{
	g_pUDA->BeginEvent(L"GBuffer");
	g_pContext->OMSetDepthStencilState(m_pDepthStencilStateGBuffer.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendStateGBuffer.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerStateGBuffer.Get());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer1);
	NXTexture2D* pGBufferRTC = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer2);
	NXTexture2D* pGBufferRTD = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer3);

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
	g_pContext->IASetInputLayout(m_pInputLayoutGBuffer.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());

	// 2022.4.14 Ö»äÖÈ¾ Opaque ÎïÌå
	for (auto pMat : m_pScene->GetMaterials())
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

void NXDeferredRenderer::Render()
{
	g_pUDA->BeginEvent(L"Deferred rendering");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilStateLighting.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendStateLighting.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerStateLighting.Get());

	auto pRTVScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	auto pDSVSceneDepth = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();
	g_pContext->OMSetRenderTargets(1, &pRTVScene, pDSVSceneDepth);

	g_pContext->VSSetShader(m_pVertexShaderRender.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderRender.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayoutRender.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());
	g_pContext->PSSetSamplers(1, 1, m_pSamplerLinearClamp.GetAddressOf());

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
		g_pContext->PSSetShaderResources(4, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(5, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(6, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pBRDF2DLUT);

		auto pIrradianceSHSRV = pCubeMap->GetSRVIrradianceSH();
		g_pContext->PSSetShaderResources(9, 1, &pIrradianceSHSRV);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(3, 1, &pCBCubeMapParam);
	}

	g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer1);
	NXTexture2D* pGBufferRTC = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer2);
	NXTexture2D* pGBufferRTD = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer3);

	ID3D11ShaderResourceView* ppSRVs[4] = {
		pGBufferRTA->GetSRV(),
		pGBufferRTB->GetSRV(),
		pGBufferRTC->GetSRV(),
		pGBufferRTD->GetSRV(),
	};

	g_pContext->PSSetShaderResources(0, 1, &ppSRVs[0]);
	g_pContext->PSSetShaderResources(1, 1, &ppSRVs[1]);
	g_pContext->PSSetShaderResources(2, 1, &ppSRVs[2]);
	g_pContext->PSSetShaderResources(3, 1, &ppSRVs[3]);

	UINT stride = sizeof(VertexPT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Release()
{
}

void NXDeferredRenderer::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
}
