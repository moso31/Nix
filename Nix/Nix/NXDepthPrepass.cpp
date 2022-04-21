#include "NXDepthPrepass.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXScene.h"
#include "NXPrefab.h"

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
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\DepthPrepass.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DepthPrepass.fx", "PS", &m_pPixelShader);

	m_pSamplerLinearWrap.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
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
	g_pContext->PSSetSamplers(0, 1, m_pSamplerLinearWrap.GetAddressOf());

	for (auto pRenderObj : m_pScene->GetRenderableObjects())
	{
		//if (pRenderObj->GetType() == NXType::ePrimitive)
		//{
		//	auto pPrim = static_cast<NXPrimitive*>(pRenderObj);
		//	pPrim->UpdateViewParams();
		//	g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

		//	for (UINT i = 0; i < pPrim->GetSubMeshCount(); i++)
		//	{
		//		auto pSubMesh = pPrim->GetSubMesh(i);
		//		pSubMesh->Update();

		//		auto pMat = pSubMesh->GetMaterial();
		//		if (pMat->IsPBRType())
		//		{
		//			NXPBRMaterialBase* pMat = static_cast<NXPBRMaterialBase*>(pSubMesh->GetMaterial());

		//			auto pSRVNormal = pMat->GetSRVNormal();
		//			g_pContext->PSSetShaderResources(0, 1, &pSRVNormal);
		//		}

		//		auto pCBMaterial = pMat->GetConstantBuffer();
		//		g_pContext->PSSetConstantBuffers(2, 1, &pCBMaterial);

		//		pSubMesh->Render();
		//	}
		//}
	}

	ID3D11RenderTargetView* nullViews[2] = { nullptr, nullptr };
	g_pContext->OMSetRenderTargets(2, nullViews, nullptr);

	g_pUDA->EndEvent();
}
