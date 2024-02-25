#include "NXGBufferRenderer.h"
#include "Global.h"
#include "Ntr.h"

#include "ShaderComplier.h"
#include "NXHLSLGenerator.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "NXAllocatorManager.h"

#include "NXPBRMaterial.h"

NXGBufferRenderer::NXGBufferRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBufferRenderer::~NXGBufferRenderer()
{
}

void NXGBufferRenderer::Init()
{
	m_pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);
	m_pGBufferRT[0] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0);
	m_pGBufferRT[1] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1);
	m_pGBufferRT[2] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2);
	m_pGBufferRT[3] = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3);

	// todo: rootparam, staticsampler. 需要结合nsl的完整逻辑想想
	m_pRootSig = NX12Util::CreateRootSignature(g_pDevice.Get(), rootParams, staticSamplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\DebugLayer.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DebugLayer.fx", "PS", pPSBlob.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutPT, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, true, D3D12_COMPARISON_FUNC_LESS, true, 0xff, 0xff, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	for (int i = 0; i < 4; i++) 
		psoDesc.RTVFormats[0] = m_pGBufferRT[0]->GetFormat();
	psoDesc.DSVFormat = m_pDepthZ->GetFormat();
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXGBufferRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "GBuffer");

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE pRTs[] = { m_pGBufferRT[0]->GetRTV(), m_pGBufferRT[1]->GetRTV(), m_pGBufferRT[2]->GetRTV(), m_pGBufferRT[3]->GetRTV() };
	m_pCommandList->ClearDepthStencilView(m_pDepthZ->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
	for (int i = 0; i < _countof(pRTs); i++)
		m_pCommandList->ClearRenderTargetView(m_pGBufferRT[i]->GetRTV(), Colors::Black, 0, nullptr);
	m_pCommandList->OMSetRenderTargets(_countof(pRTs), pRTs, false, &m_pDepthZ->GetDSV());

	auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
	auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
	for (auto pMat : pMaterialsArray)
	{
		auto pEasyMat = pMat ? pMat->IsEasyMat() : nullptr;
		auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
		m_pCommandList->OMSetStencilRef(0x0);

		if (pEasyMat)
		{
			pEasyMat->Render();
			for (auto pSubMesh : pEasyMat->GetRefSubMeshes())
			{
				if (pSubMesh)
				{
					bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
					if (bIsVisible)
					{
						pSubMesh->UpdateViewParams();
						m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBufferManager::m_cbDataObject.Current().GPUVirtualAddr);
						pSubMesh->Update();
						pSubMesh->Render(m_pCommandList.Get());
					}
				}
			}
		}
		else if (pCustomMat)
		{
			if (pCustomMat->GetCompileSuccess())
			{
				if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
				{
					// 3S材质需要写模板缓存
					m_pCommandList->OMSetStencilRef(0x1);
				}

				pCustomMat->Render();

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->UpdateViewParams();
							m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBufferManager::m_cbDataObject.Current().GPUVirtualAddr);
							pSubMesh->Update();
							pSubMesh->Render(m_pCommandList.Get());
						}
					}
				}
			}
			else
			{
				pErrorMat->Render();

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->UpdateViewParams();
							m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBufferManager::m_cbDataObject.Current().GPUVirtualAddr);
							pSubMesh->Update();
							pSubMesh->Render(m_pCommandList.Get());
						}
					}
				}
			}
		}
	}

	NX12Util::EndEvent();
}

void NXGBufferRenderer::Release()
{
}
