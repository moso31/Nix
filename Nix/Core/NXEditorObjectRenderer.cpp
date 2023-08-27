#include "NXEditorObjectRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"
#include "NXRenderTarget.h"
#include "NXScene.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"

NXEditorObjectRenderer::NXEditorObjectRenderer(NXScene* pScene) :
	m_pScene(pScene),
	m_pRTQuad(nullptr),
	m_pPassOutTex(nullptr)
{
}

NXEditorObjectRenderer::~NXEditorObjectRenderer()
{
}

void NXEditorObjectRenderer::Init()
{
	m_pRTQuad = new NXRenderTarget();
	m_pRTQuad->Init();

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\EditorObjects.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutEditorObject, ARRAYSIZE(NXGlobalInputLayout::layoutEditorObject), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\EditorObjects.fx", "PS", &m_pPixelShader);

	m_pDepthStencilState = NXDepthStencilState<false, false, D3D11_COMPARISON_LESS>::Create();
	m_pRasterizerState = NXRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::Create();
	m_pBlendState = NXBlendState<false, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD>::Create();

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferParams_Internal);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbParams));
}

void NXEditorObjectRenderer::OnResize(const Vector2& rtSize)
{
	if (m_pPassOutTex)
		m_pPassOutTex->RemoveRef();

	m_pPassOutTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Editor objects Out RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
	m_pPassOutTex->AddRTV();
	m_pPassOutTex->AddSRV();
}

void NXEditorObjectRenderer::Render()
{
	g_pUDA->BeginEvent(L"Editor objects");

	g_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
	g_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
	g_pContext->RSSetState(m_pRasterizerState.Get());

	auto pRTVOutput = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing)->GetRTV();
	g_pContext->OMSetRenderTargets(1, &pRTVOutput, nullptr);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	g_pContext->VSSetConstantBuffers(1, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());

  	NXEditorObjectManager* pEditorObjManager = m_pScene->GetEditorObjManager();
	for (auto pEditObj : pEditorObjManager->GetEditableObjects())
	{
		if (pEditObj->GetVisible()) // if bIsVisible
		{
			for (UINT i = 0; i < pEditObj->GetSubMeshCount(); i++)
			{
				auto pSubMesh = pEditObj->GetSubMesh(i);
				if (pSubMesh->IsSubMeshEditorObject())
				{
					NXSubMeshEditorObjects* pSubMeshEditorObj = (NXSubMeshEditorObjects*)pSubMesh;

					// 判断当前SubMesh是否高亮显示
					{
						bool bIsHighLight = pSubMeshEditorObj->GetEditorObjectID() == m_pScene->GetEditorObjManager()->GetHighLightID();
						m_cbDataParams.params.x = bIsHighLight ? 1.0f : 0.0f;
						g_pContext->UpdateSubresource(m_cbParams.Get(), 0, nullptr, &m_cbDataParams, 0, 0);
						g_pContext->PSSetConstantBuffers(2, 1, m_cbParams.GetAddressOf());
					}

					pSubMeshEditorObj->UpdateViewParams();
					g_pContext->VSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbObject.GetAddressOf());

					pSubMeshEditorObj->Update();
					pSubMeshEditorObj->Render();
				}
			}
		}
	}

	g_pUDA->EndEvent();
}

void NXEditorObjectRenderer::Release()
{
	if (m_pPassOutTex) m_pPassOutTex->RemoveRef();
	SafeRelease(m_pRTQuad);
}
