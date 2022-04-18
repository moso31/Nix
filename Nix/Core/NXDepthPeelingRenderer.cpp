#include "NXDepthPeelingRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"

#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderTarget.h"
#include "NXCamera.h"

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
	for (UINT i = 0; i < m_peelingLayerCount; i++)
	{
		m_pSceneRT[i] = NXResourceManager::GetInstance()->CreateTexture2D("Depth Peeling Scene RT " + std::to_string(i), DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
		m_pSceneRT[i]->CreateRTV();
		m_pSceneRT[i]->CreateSRV();
	}

	m_pCombineRTData = new NXRenderTarget();
	m_pCombineRTData->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ForwardTranslucent.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ForwardTranslucent.fx", "PS", &m_pPixelShader);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\ForwardTranslucent.fx", "VS", &m_pVertexShaderDepthPeeling, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);

	NXShaderComplier::GetInstance()->AddMacro(CD3D_SHADER_MACRO("ENABLE_DEPTH_PEELING", "1"));
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\ForwardTranslucent.fx", "PS", &m_pPixelShaderDepthPeeling);

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DepthPeelingCombine.fx", "VS", &m_pVertexShaderCombine, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayoutCombine);

	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DepthPeelingCombine.fx", "PS", &m_pPixelShaderCombine);

	m_pDepthStencilState = NXDepthStencilState<>::Create();
	m_pRasterizerStateFront = NXRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK>::Create();
	m_pRasterizerStateBack = NXRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_FRONT>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA>::Create();
	m_pBlendStateOpaque = NXBlendState<>::Create();

	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	m_pSamplerLinearClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
	m_pSamplerPointClamp.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());

	InitConstantBuffer();
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
	g_pContext->RSSetState(m_pRasterizerStateFront.Get());

	for (UINT i = 0; i < m_peelingLayerCount; i++)
	{
		g_pUDA->BeginEvent(L"Layer");

		// ǰ����ò�͸����Ⱦ
		// ���һ��ǿ�ư�͸��Ⱦ���ܱȿ�����Ҫ�ã�
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
			g_pContext->VSSetShader(m_pVertexShaderDepthPeeling.Get(), nullptr, 0);
			g_pContext->PSSetShader(m_pPixelShaderDepthPeeling.Get(), nullptr, 0);
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

		// PBR��ġ���Ӱ��ͼ��ʱͣ�á�
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

		RenderLayer();

		ID3D11ShaderResourceView* const pNullSRV[1] = { nullptr };
		g_pContext->PSSetShaderResources(11, 1, pNullSRV);

		g_pUDA->EndEvent();
	}

	// Combine Layers
	g_pUDA->BeginEvent(L"Combine");

	// �� cb params
	{
		g_pContext->PSSetConstantBuffers(4, 1, m_cbDepthPeelingParams.GetAddressOf());
	}

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerStateFront.Get());

	g_pContext->VSSetShader(m_pVertexShaderCombine.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderCombine.Get(), nullptr, 0);

	g_pContext->IASetInputLayout(m_pInputLayoutCombine.Get());

	g_pContext->PSSetSamplers(0, 1, m_pSamplerPointClamp.GetAddressOf());

	auto pRTVMainScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetRTV();
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, nullptr);

	for (UINT i = 0; i < m_peelingLayerCount; i++)
	{
		auto pSRVScene = m_pSceneRT[i]->GetSRV();
		g_pContext->PSSetShaderResources(i, 1, &pSRVScene);
	}

	// ����TMҪ�������NullSRV����Ҫ��ô��������������������
	for (UINT i = m_peelingLayerCount; i < 11; i++)
	{
		ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
		g_pContext->PSSetShaderResources(i, 1, pNullSRV);
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

void NXDepthPeelingRenderer::InitConstantBuffer()
{
	m_cbDepthPeelingParamsData.depthLayer = m_peelingLayerCount;
	m_cbDepthPeelingParamsData._0 = Vector3(0.0f);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferDepthPeelingParams);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbDepthPeelingParams));

	g_pContext->UpdateSubresource(m_cbDepthPeelingParams.Get(), 0, nullptr, &m_cbDepthPeelingParamsData, 0, 0);
}

void NXDepthPeelingRenderer::RenderLayer()
{
	auto pMainCamera = m_pScene->GetMainCamera();
	Vector3 cameraPos = pMainCamera ? Vector3(0.0f) : pMainCamera->GetTranslation();

	// 2022.4.14 ֻ��Ⱦ Transparent ����
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

			// 2022.4.18 
			// ����������Զ�������򣬾��������͸��Ⱦ͸�ӹ�ϵ��������⵫�������ޡ���Ҫ���ǵÿ�OIT��
			auto& subMeshes = pPBRMat->GetRefSubMeshes();
			std::sort(subMeshes.begin(), subMeshes.end(), [cameraPos](NXSubMesh* meshA, NXSubMesh* meshB) { 
				Vector3 posA = meshA->GetPrimitive()->GetTranslation();
				Vector3 posB = meshB->GetPrimitive()->GetTranslation();
				float distA = Vector3::Distance(posA, cameraPos);
				float distB = Vector3::Distance(posB, cameraPos);
				return distA > distB;
			});

			for (auto pSubMesh : subMeshes)
			{
				if (pSubMesh)
				{
					pSubMesh->UpdateViewParams();
					g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

					pSubMesh->Update();

					// ��Ⱦ���飬��Զ���
					g_pContext->RSSetState(m_pRasterizerStateBack.Get());
					pSubMesh->Render();
					g_pContext->RSSetState(m_pRasterizerStateFront.Get());
					pSubMesh->Render();
				}
			}
		}
	}
}
