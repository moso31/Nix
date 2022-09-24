#include "NXEditorObjectRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXRenderTarget.h"
#include "NXScene.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXPrimitive.h"

NXEditorObjectRenderer::NXEditorObjectRenderer(NXScene* pScene) :
	m_pScene(pScene),
	m_pRTQuad(nullptr)
{
}

NXEditorObjectRenderer::~NXEditorObjectRenderer()
{
}

void NXEditorObjectRenderer::Init()
{
	m_pRTQuad = new NXRenderTarget();
	m_pRTQuad->Init();

	Vector2 sz = g_dxResources->GetViewSize();
	m_pPassOutTex = NXResourceManager::GetInstance()->CreateTexture2D("Editor objects Out RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
	m_pPassOutTex->AddRTV();
	m_pPassOutTex->AddSRV();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\EditorObjects.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutEditorObject, ARRAYSIZE(NXGlobalInputLayout::layoutEditorObject), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\EditorObjects.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<false, false, D3D11_COMPARISON_LESS>::Create();
	m_pRasterizerState = NXRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD>::Create();
}

void NXEditorObjectRenderer::Render()
{
	g_pUDA->BeginEvent(L"Editor objects");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVOutput = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_PostProcessing)->GetRTV();
	g_pContext->OMSetRenderTargets(1, &pRTVOutput, nullptr);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

	for (auto pEditObj : m_pScene->GetEditableObjects())
	{
		if (pEditObj->GetVisible()) // if bIsVisible
		{
			for (UINT i = 0; i < pEditObj->GetSubMeshCount(); i++)
			{
				auto pSubMesh = pEditObj->GetSubMesh(i);
				pSubMesh->UpdateViewParams();
				g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

				pSubMesh->Update();
				pSubMesh->Render();
			}
		}
	}

	g_pUDA->EndEvent();
}

void NXEditorObjectRenderer::Release()
{
	SafeDelete(m_pPassOutTex);
	SafeRelease(m_pRTQuad);
}
