#include "NXGBufferRenderer.h"
#include "NXGlobalDefinitions.h"
#include "Ntr.h"

#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

#include "NXScene.h"
#include "NXCamera.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXAllocatorManager.h"
#include "NXGPUTerrainManager.h"
#include "NXPBRMaterial.h"

NXGBufferRenderer::NXGBufferRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXGBufferRenderer::~NXGBufferRenderer()
{
}

void NXGBufferRenderer::SetCamera(NXCamera* pCamera)
{
	m_pCamera = pCamera;
}

void NXGBufferRenderer::Render()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();
	NX12Util::BeginEvent(pCmdList, "GBuffer");

	NXGraphicPass::RenderSetTargetAndState(pCmdList);

	auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
	auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
	for (auto pMat : pMaterialsArray)
	{
		auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
		pCmdList->OMSetStencilRef(0x0);

		if (pMat)
		{
			// 更新材质CB
			pMat->Update(); 
		}

		if (pCustomMat && pCustomMat->GetCompileSuccess())
		{
			if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
			{
				// 3S材质需要写模板缓存
				pCmdList->OMSetStencilRef(0x1);
			}
		}

		pMat->Render(pCmdList);
		m_pCamera->Render(pCmdList);
		for (auto pSubMesh : pMat->GetRefSubMeshes())
		{
			if (pSubMesh)
			{
				bool bIsVisible = pSubMesh->GetRenderableObject()->GetVisible();
				if (bIsVisible)
				{
					if (pSubMesh->IsSubMeshTerrain())
					{
						NXGPUTerrainManager::GetInstance()->UpdateConstantForGBuffer(pCmdList);
						pSubMesh->Render(pCmdList);
						break;
					}

					pSubMesh->GetRenderableObject()->Update(pCmdList); // 永远优先调用派生类的Update 
					pSubMesh->Render(pCmdList);
				}
			}
		}
	}

	NX12Util::EndEvent(pCmdList);
}

void NXGBufferRenderer::Release()
{
}
