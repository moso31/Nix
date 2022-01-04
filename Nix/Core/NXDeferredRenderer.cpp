#include "NXDeferredRenderer.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"

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

	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	// 对RT0-RT3使用LayoutPNTT顶点布局。
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutGBuffer));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DeferredRender.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShaderRender));
	
	// 对最终渲染使用LayoutPT顶点布局。
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayoutRender));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\GBuffer.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DeferredRender.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShaderRender));
}

void NXDeferredRenderer::RenderGBuffer()
{
	g_pContext->IASetInputLayout(m_pInputLayoutGBuffer.Get());

	g_pUDA->BeginEvent(L"GBuffer");

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

	g_pContext->OMSetRenderTargets(4, ppRTVs, pDepthZ->GetDSV());

	for (int i = 0; i < 4; i++)
		g_pContext->ClearRenderTargetView(ppRTVs[i], Colors::Black);
	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 设置使用的VS和PS（scene.fx）
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	for (auto pPrim : m_pScene->GetPrimitives())
	{
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

	ID3D11RenderTargetView* nullViews[4] = { nullptr, nullptr, nullptr, nullptr };
	g_pContext->OMSetRenderTargets(4, nullViews, nullptr);

	g_pUDA->EndEvent();
}

void NXDeferredRenderer::Render(ID3D11ShaderResourceView* pSRVSSAO)
{
	g_pUDA->BeginEvent(L"Deferred rendering");

	auto pRTVMainScene = g_dxResources->GetRTVMainScene();
	auto pDSVDepthStencil = g_dxResources->GetDSVDepthStencil();
	g_pContext->OMSetRenderTargets(1, &pRTVMainScene, pDSVDepthStencil);
	g_pContext->ClearRenderTargetView(pRTVMainScene, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDSVDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->IASetInputLayout(m_pInputLayoutRender.Get());
	g_pContext->VSSetShader(m_pVertexShaderRender.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShaderRender.Get(), nullptr, 0);
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
		g_pContext->PSSetShaderResources(4, 1, &pCubeMapSRV);
		g_pContext->PSSetShaderResources(5, 1, &pIrradianceMapSRV);
		g_pContext->PSSetShaderResources(6, 1, &pPreFilterMapSRV);
		g_pContext->PSSetShaderResources(7, 1, &pBRDF2DLUT);

		auto pCBCubeMapParam = pCubeMap->GetConstantBufferParams();
		g_pContext->PSSetConstantBuffers(3, 1, &pCBCubeMapParam);
	}

	if (pSRVSSAO)
	{
		g_pContext->PSSetShaderResources(8, 1, &pSRVSSAO);
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

	auto pTexDepthStencil = g_dxResources->GetTexDepthStencil();
	g_pContext->CopyResource(pTexDepthStencil, pDepthZ->GetTex());

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
