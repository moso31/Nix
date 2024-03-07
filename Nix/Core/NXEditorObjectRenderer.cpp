#include "BaseDefs/NixCore.h"

#include "NXEditorObjectRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXAllocatorManager.h"
#include "NXScene.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"

NXEditorObjectRenderer::NXEditorObjectRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXEditorObjectRenderer::~NXEditorObjectRenderer()
{
}

void NXEditorObjectRenderer::Init()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\EditorObjects.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\EditorObjects.fx", "PS", pPSBlob.Get());

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.push_back(NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL));
	rootParams.push_back(NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL));
	rootParams.push_back(NX12Util::CreateRootParameterCBV(2, 0, D3D12_SHADER_VISIBILITY_ALL));

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutEditorObject;
	psoDesc.BlendState = NXBlendState<false, false, true, false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD>::Create();
	psoDesc.RasterizerState = NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPassOut->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		NXAllocatorManager::GetInstance()->GetCBufferAllocator()->Alloc(ResourceType_Upload, m_cbParams.Get(i));
	}

	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_PostProcessing);
}

void NXEditorObjectRenderer::OnResize(const Vector2& rtSize)
{
}

void NXEditorObjectRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Editor objects");

	m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbCamera.Current().GPUVirtualAddr);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	m_pCommandList->OMSetRenderTargets(1, &m_pTexPassOut->GetRTV(), false, nullptr);

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
						m_cbParams.Current().data.value.x = bIsHighLight ? 1.0f : 0.0f;
						NXAllocatorManager::GetInstance()->GetCBufferAllocator()->UpdateData(m_cbParams.Current());
						m_pCommandList->SetGraphicsRootConstantBufferView(0, m_cbParams.Current().GPUVirtualAddr);
					}

					pSubMeshEditorObj->UpdateViewParams();

					m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.Current().GPUVirtualAddr);

					pSubMeshEditorObj->Update();
					pSubMeshEditorObj->Render(m_pCommandList.Get());
				}
			}
		}
	}

	NX12Util::EndEvent();
}

void NXEditorObjectRenderer::Release()
{
}
