#include "NXVirtualTextureRenderer.h"

void NXVTReadbackRenderer::SetupInternal()
{
	SetPassName("Fill Test");
	SetShaderFilePath(L"Shader\\VTReadback.fx");

	InitCSO();
}
