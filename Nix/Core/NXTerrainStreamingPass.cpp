#include "NXTerrainStreamingPass.h"

void NXTerrainStreamingPass::SetupInternal()
{
	SetPassName("????");
	SetShaderFilePath(L"Shader\\?????.fx");
	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitCSO();
}
