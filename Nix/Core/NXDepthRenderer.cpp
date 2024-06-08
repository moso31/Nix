#include "NXDepthRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"
#include "NXResourceManager.h"
#include "NXSamplerManager.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

void NXDepthRenderer::Init()
{
	AddInputTex(NXCommonRT_DepthZ);
	AddOutputRT(NXCommonRT_DepthZ_R32);

	// t0, s0, b is empty
	SetShaderFilePath(L"Shader\\Depth.fx");
	SetRootParams(0, 1); // param 0 = t0
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS, true, 0xFF, 0xFF, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL>::Create());

	InitPSO();
}

void NXDepthRenderer::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, "Depth Copy");
	RenderBegin(pCmdList);
	pCmdList->OMSetStencilRef(0x01);
	RenderEnd(pCmdList);
	pCmdList->OMSetStencilRef(0x00);
	NX12Util::EndEvent(pCmdList);
}

void NXDepthRenderer::Release()
{
}
