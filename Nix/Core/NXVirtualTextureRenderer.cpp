#include "NXVirtualTextureRenderer.h"

void NXVTReadbackRenderer::SetupInternal()
{
	SetPassName("VT Readback");
	SetShaderFilePath(L"Shader\\VTReadback.fx");

	InitCSO();
}
