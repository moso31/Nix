#include "NXFillTestRenderer.h"

void NXFillTestRenderer::SetupInternal()
{
	SetPassName("Fill Test");
	SetShaderFilePath(L"Shader\\FillTestComputeShader.fx");
	SetRootParams(0, 0, 1);
	InitCSO();
}
