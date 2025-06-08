#include "NXFillTestRenderer.h"
#include "NXGPUTerrainManager.h"

void NXFillTestRenderer::SetupInternal()
{
	SetPassName("Fill Test");
	SetShaderFilePath(L"Shader\\FillTestComputeShader.fx");

	SetStaticRootParamCBV(0, 0, &NXGPUTerrainManager::GetInstance()->GetTerrainParams().GetFrameGPUAddresses());
	InitCSO();
}
