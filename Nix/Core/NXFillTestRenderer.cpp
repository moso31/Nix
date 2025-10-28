#include "NXFillTestRenderer.h"

void NXFillTestRenderer::SetupInternal()
{
	SetShaderFilePath(L"Shader\\FillTestComputeShader.fx");

	InitCSO();
}

void NXGPUTerrainPatcherRenderer::SetupInternal()
{
	SetShaderFilePath(L"Shader\\GPUTerrainPatcher.fx");
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitCSO();
}
