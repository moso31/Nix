#include "NXForwardRenderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "RenderStates.h"

#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXForwardRenderer::NXForwardRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXForwardRenderer::~NXForwardRenderer()
{
}

void NXForwardRenderer::Init()
{
	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Scene.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));
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

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		// 这里渲染场景中所有Mesh。
		// 其他几个CB/SRV的Slot都不变，但每个物体的World、Material、Tex信息都可能改变。
		// 【可以进一步优化】按Material绘制、按Tex绘制。
		pPrim->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		for (UINT i = 0; i < pPrim->GetSubMeshCount(); i++)
		{
			auto pSubMesh = pPrim->GetSubMesh(i);
			pSubMesh->Update();

			auto pMat = pSubMesh->GetPBRMaterial();
			auto pSRVAlbedo = pMat->GetSRVAlbedo();
			g_pContext->PSSetShaderResources(1, 1, &pSRVAlbedo);

			auto pSRVNormal = pMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(2, 1, &pSRVNormal);

			auto pSRVMetallic = pMat->GetSRVMetallic();
			g_pContext->PSSetShaderResources(3, 1, &pSRVMetallic);

			auto pSRVRoughness = pMat->GetSRVRoughness();
			g_pContext->PSSetShaderResources(4, 1, &pSRVRoughness);

			auto pSRVAO = pMat->GetSRVAO();
			g_pContext->PSSetShaderResources(5, 1, &pSRVAO);

			auto pCBMaterial = pMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(3, 1, &pCBMaterial);

			pSubMesh->Render();
		}
	}

	g_pUDA->EndEvent();
}
