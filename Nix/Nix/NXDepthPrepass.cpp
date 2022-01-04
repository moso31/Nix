#include "NXDepthPrepass.h"
#include "ShaderComplier.h"
#include "RenderStates.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"

NXDepthPrepass::NXDepthPrepass(NXScene* pScene) :
	m_pInputLayout(nullptr),
	m_pScene(pScene)
{
}

NXDepthPrepass::~NXDepthPrepass()
{
}

void NXDepthPrepass::Init(const Vector2& DepthBufferSize)
{
	// create VS & IL
	ComPtr<ID3DBlob> pVSBlob;
	ComPtr<ID3DBlob> pPSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout));

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\DepthPrepass.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));
}

void NXDepthPrepass::Render()
{
	g_pUDA->BeginEvent(L"Depth Prepass");

	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	NXTexture2D* pDepthZ = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ);
	NXTexture2D* pGBufferRTA = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer0);
	NXTexture2D* pGBufferRTB = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_GBuffer1);

	ID3D11RenderTargetView* ppRTVs[2] = {
		pGBufferRTA->GetRTV(),
		pGBufferRTB->GetRTV(),
	};

	g_pContext->OMSetRenderTargets(2, ppRTVs, pDepthZ->GetDSV());
	g_pContext->ClearRenderTargetView(ppRTVs[0], Colors::Black);
	g_pContext->ClearRenderTargetView(ppRTVs[1], Colors::Black);
	g_pContext->ClearDepthStencilView(pDepthZ->GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->PSSetSamplers(0, 1, RenderStates::SamplerLinearWrap.GetAddressOf());

	for (auto pPrim : m_pScene->GetPrimitives())
	{
		pPrim->UpdateViewParams();
		g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		for (UINT i = 0; i < pPrim->GetSubMeshCount(); i++)
		{
			auto pSubMesh = pPrim->GetSubMesh(i);
			pSubMesh->Update();

			auto pMat = pSubMesh->GetPBRMaterial();
			auto pSRVNormal = pMat->GetSRVNormal();
			g_pContext->PSSetShaderResources(0, 1, &pSRVNormal);

			auto pCBMaterial = pMat->GetConstantBuffer();
			g_pContext->PSSetConstantBuffers(2, 1, &pCBMaterial);

			pSubMesh->Render();
		}
	}

	ID3D11RenderTargetView* nullViews[2] = { nullptr, nullptr };
	g_pContext->OMSetRenderTargets(2, nullViews, nullptr);

	g_pUDA->EndEvent();
}
