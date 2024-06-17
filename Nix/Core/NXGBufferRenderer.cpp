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
}

void NXGBufferRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "GBuffer");

	Ntr<NXTexture2D> pGBuffers[] =
	{
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3),
	};
	Ntr<NXTexture2D> pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	D3D12_CPU_DESCRIPTOR_HANDLE pRTs[] = { pGBuffers[0]->GetRTV(), pGBuffers[1]->GetRTV(), pGBuffers[2]->GetRTV(), pGBuffers[3]->GetRTV() };
	pCmdList->ClearDepthStencilView(pDepthZ->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
	for (int i = 0; i < _countof(pRTs); i++)
		pCmdList->ClearRenderTargetView(pGBuffers[i]->GetRTV(), Colors::Black, 0, nullptr);
	pCmdList->OMSetRenderTargets(_countof(pRTs), pRTs, false, &pDepthZ->GetDSV());

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
	auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
	for (auto pMat : pMaterialsArray)
	{
		auto pEasyMat = pMat ? pMat->IsEasyMat() : nullptr;
		auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
		pCmdList->OMSetStencilRef(0x0);

		if (pEasyMat)
		{
			pEasyMat->Render(pCmdList);
			for (auto pSubMesh : pEasyMat->GetRefSubMeshes())
			{
				if (pSubMesh)
				{
					bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
					if (bIsVisible)
					{
						pSubMesh->GetPrimitive()->Update(pCmdList);
						pSubMesh->Update(pCmdList);
						pSubMesh->Render(pCmdList);
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
					pCmdList->OMSetStencilRef(0x1);
				}

				pCustomMat->Render(pCmdList);

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->GetPrimitive()->Update(pCmdList);
							pSubMesh->Update(pCmdList);
							pSubMesh->Render(pCmdList);
						}
					}
				}
			}
			else
			{
				pErrorMat->Render(pCmdList);

				for (auto pSubMesh : pCustomMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetPrimitive()->GetVisible();
						if (bIsVisible)
						{
							pSubMesh->GetPrimitive()->Update(pCmdList);
							pSubMesh->Update(pCmdList);
							pSubMesh->Render(pCmdList);
						}
					}
				}
			}
		}
	}

	NX12Util::EndEvent(pCmdList);
}

void NXGBufferRenderer::Release()
{
}
