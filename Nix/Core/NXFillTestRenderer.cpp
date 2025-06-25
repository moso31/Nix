#include "NXFillTestRenderer.h"
#include "NXGPUTerrainManager.h"

void NXFillTestRenderer::SetupInternal()
{
	SetPassName("Fill Test");
	SetShaderFilePath(L"Shader\\FillTestComputeShader.fx");

	InitCSO();
}

void NXGPUTerrainPatcherRenderer::SetupInternal()
{
	SetPassName("GPUTerrainPatcher");
	SetShaderFilePath(L"Shader\\GPUTerrainPatcher.fx");
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitCSO();
}
