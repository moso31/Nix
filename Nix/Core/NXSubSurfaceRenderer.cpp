#include "NXSubSurfaceRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXGlobalBuffers.h"
#include "NXResourceManager.h"
#include "NXSamplerManager.h"
#include "NXTexture.h"
#include "NXScene.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

NXSubSurfaceRenderer::NXSubSurfaceRenderer()
{
}

NXSubSurfaceRenderer::~NXSubSurfaceRenderer()
{
}

void NXSubSurfaceRenderer::SetupInternal()
{
	SetPassName("Burley 3S");

	SetShaderFilePath(L"Shader\\SSSSSRenderer.fx");
	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS, true, 0xFF, 0xFF, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL>::Create());

	// t0~t5, s0, b3
	SetRootParams(2, 6);
	SetStaticRootParamCBV(0, 1, &g_cbCamera.GetFrameGPUAddresses());
	SetStaticRootParamCBV(1, 3, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	SetStencilRef(0x1);

	InitPSO();
}
