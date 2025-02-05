#include "NXShadowTestRenderer.h"
#include "NXRenderStates.h"
#include "NXGlobalBuffers.h"

void NXShadowTestRenderer::SetupInternal()
{
	SetPassName("Shadow Test");
	SetShaderFilePath("Shader\\ShadowTest.fx");
	SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create());
	SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
	SetRootParams(3, 2);
	SetStaticRootParamCBV(0, &g_cbObject.GetFrameGPUAddresses());
	SetStaticRootParamCBV(1, &g_cbCamera.GetFrameGPUAddresses());
	SetStaticRootParamCBV(2, &g_cbShadowTest.GetFrameGPUAddresses());
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	InitPSO();
}
