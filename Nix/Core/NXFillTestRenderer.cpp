#include "NXFillTestRenderer.h"

void NXFillTestRenderer::SetupInternal()
{
	auto pMat = GetPassMaterial();
	pMat->RegisterCBVSpaceNum(1);
	pMat->RegisterCBVSlotNum(1);
	pMat->RegisterUAVSpaceNum(1);
	pMat->RegisterUAVSpaceNum(3);
	pMat->FinalizeLayout();
	pMat->SetShaderFilePath(L"Shader\\FillTestComputeShader.fx");
}

void NXGPUTerrainPatcherRenderer::SetupInternal()
{
	auto pMat = GetPassMaterial();
	pMat->RegisterCBVSpaceNum(1);
	pMat->RegisterCBVSlotNum(2);
	pMat->RegisterSRVSpaceNum(1);
	pMat->RegisterSRVSlotNum(2);
	pMat->RegisterUAVSpaceNum(1);
	pMat->RegisterUAVSlotNum(3);
	pMat->FinalizeLayout();

	pMat->SetShaderFilePath(L"Shader\\GPUTerrainPatcher.fx");
	pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
}
