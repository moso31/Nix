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
	SetPassName("Editor Objects");
	AddOutputRT(NXCommonRT_PostProcessing);

	SetShaderFilePath(L"Shader\\EditorObjects.fx");
	SetBlendState(NXBlendState<false, false, true, false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD>::Create());
	SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE>::Create());
	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS>::Create());
	SetInputLayout(NXGlobalInputLayout::layoutEditorObject);

	SetRootParams(3, 0); // b0~b2. (actually do not need b1.)
	SetStaticRootParamCBV(1, NXGlobalBuffer::cbCamera.GetGPUHandleArray());

	InitPSO();
}

void NXEditorObjectRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "Editor objects");

	NXRendererPass::RenderBefore(pCmdList);

  	NXEditorObjectManager* pEditorObjManager = m_pScene->GetEditorObjManager();
	for (auto pEditObj : pEditorObjManager->GetEditableObjects())
	{
		if (pEditObj->GetVisible()) // if bIsVisible
		{
			pEditObj->Update(pCmdList); // b0 update in here.

			for (UINT i = 0; i < pEditObj->GetSubMeshCount(); i++)
			{
				auto pSubMesh = pEditObj->GetSubMesh(i);
				if (pSubMesh->IsSubMeshEditorObject())
				{
					NXSubMeshEditorObjects* pSubMeshEditorObj = (NXSubMeshEditorObjects*)pSubMesh;
					bool bIsHighLight = pSubMeshEditorObj->GetEditorObjectID() == m_pScene->GetEditorObjManager()->GetHighLightID();
					pSubMeshEditorObj->Update(pCmdList, bIsHighLight);
					pSubMeshEditorObj->Render(pCmdList);
				}
			}
		}
	}

	NX12Util::EndEvent(pCmdList);
}

void NXEditorObjectRenderer::Release()
{
}
