#include "BaseDefs/NixCore.h"

#include "NXColorMappingRenderer.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "NXResourceManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"
#include "NXSamplerManager.h"

NXColorMappingRenderer::NXColorMappingRenderer() 
{
}

NXColorMappingRenderer::~NXColorMappingRenderer()
{
}

void NXColorMappingRenderer::SetupInternal()
{
	SetShaderFilePath(L"Shader\\ColorMapping.fx");

	SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());

	AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	InitPSO();
}

void NXColorMappingRenderer::Release()
{
}
