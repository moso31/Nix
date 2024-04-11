#include "NXGBufferRenderer.h"
#include "NXGlobalDefinitions.h"
#include "Ntr.h"

#include "ShaderComplier.h"
#include "NXHLSLGenerator.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
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
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\DebugLayer.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\DebugLayer.fx", "PS", pPSBlob.GetAddressOf());
}

void NXGBufferRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "GBuffer");

	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	Ntr<NXTexture2D> pGBuffers[] =
	{
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3),
	};
	Ntr<NXTexture2D> pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	D3D12_CPU_DESCRIPTOR_HANDLE pRTs[] = { pGBuffers[0]->GetRTV(), pGBuffers[1]->GetRTV(), pGBuffers[2]->GetRTV(), pGBuffers[3]->GetRTV() };
	m_pCommandList->ClearDepthStencilView(pDepthZ->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
	for (int i = 0; i < _countof(pRTs); i++)
		m_pCommandList->ClearRenderTargetView(pGBuffers[i]->GetRTV(), Colors::Black, 0, nullptr);
	m_pCommandList->OMSetRenderTargets(_countof(pRTs), pRTs, false, &pDepthZ->GetDSV());

	auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
	auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
	for (auto pMat : pMaterialsArray)
	{
		auto pEasyMat = pMat ? pMat->IsEasyMat() : nullptr;
		auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
		m_pCommandList->OMSetStencilRef(0x0);

		if (pEasyMat)
		{
			pEasyMat->Render(m_pCommandList.Get());
			for (auto pSubMesh : pEasyMat->GetRefSubMeshes())
			{
				if (pSubMesh)
				{
					bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
					if (bIsVisible)
					{
						pSubMesh->UpdateViewParams();
						m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.GetGPUHandle());
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

				pCustomMat->Render(m_pCommandList.Get());

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->UpdateViewParams();
							m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.GetGPUHandle());
							pSubMesh->Update();
							pSubMesh->Render(m_pCommandList.Get());
						}
					}
				}
			}
			else
			{
				pErrorMat->Render(m_pCommandList.Get());

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->UpdateViewParams();
							m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.GetGPUHandle());
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
