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

	InitCSO();
}
