#include "NXDebugLayerRenderer.h"
#include "NXShadowMapRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXGlobalBuffers.h"

NXDebugLayerRenderer::NXDebugLayerRenderer()
{
}

void NXDebugLayerRenderer::SetupInternal()
{
	SetPassName("Debug Layer");

	SetShaderFilePath(L"Shader\\DebugLayer.fx");
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s0

	InitPSO();
}
