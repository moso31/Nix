#include "NXPassMaterialManager.h"
#include "NXPassMaterial.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"

NXPassMaterialManager* NXPassMaterialManager::GetInstance()
{
	static NXPassMaterialManager instance;
	return &instance;
}

void NXPassMaterialManager::Init()
{
	// Terrain Atlas Baker
	{
		auto pMat = new NXComputePassMaterial("TerrainAtlasBaker", L"Shader\\TerrainAtlasBaker.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(1);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(4);
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["TerrainAtlasBaker"] = pMat;
	}

	// TerrainFillTest
	{
		auto pMat = new NXComputePassMaterial("TerrainFillTest", L"Shader\\FillTestComputeShader.fx");
		pMat->SetEntryNameCS(L"CS_Pass");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);  // b0
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);  // u0, u1, u2
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["TerrainFillTest"] = pMat;
	}

	// TerrainGPUPatcher Clear
	{
		auto pMat = new NXComputePassMaterial("TerrainGPUPatcher", L"Shader\\GPUTerrainPatcher.fx");
		pMat->SetEntryNameCS(L"CS_Clear");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0, b1, b2
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);  // t0, t1
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);  // u0, u1, u2
		pMat->FinalizeLayout();
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["TerrainGPUPatcher:clear"] = pMat;
	}

	// TerrainGPUPatcher Patch
	{
		auto pMat = new NXComputePassMaterial("TerrainGPUPatcher", L"Shader\\GPUTerrainPatcher.fx");
		pMat->SetEntryNameCS(L"CS_Patch");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0, b1, b2
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);  // t0, t1
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);  // u0, u1, u2
		pMat->FinalizeLayout();
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["TerrainGPUPatcher:patch"] = pMat;
	}

	// GBuffer（GBuffer由Mesh提供CBV SRV，这里不需要）
	{
		auto pMat = new NXGraphicPassMaterial("GBuffer");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM });
		pMat->RegisterDSV({ DXGI_FORMAT_R24G8_TYPELESS });
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["GBuffer"] = pMat;
	}

	// VTReadback computeshader
	{
		auto pMat = new NXComputePassMaterial("VTReadback", L"Shader\\VTReadback.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);  // b0
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);  // t0
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(1);  // u0
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["VTReadback"] = pMat;
	}

	// VTReadback dataout
	{
		auto pMat = new NXReadbackPassMaterial("VTReadbackData");
		m_pPassMaterialMaps["VTReadbackData"] = pMat;
	}

	// ShadowMap
	{
		auto pMat = new NXGraphicPassMaterial("ShadowMap", L"Shader\\ShadowMap.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);
		pMat->RegisterDSV(DXGI_FORMAT_R32_TYPELESS);
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["ShadowMap"] = pMat;
	}

	// ShadowTest
	{
		auto pMat = new NXGraphicPassMaterial("ShadowTest", L"Shader\\ShadowTest.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);  // t0-t1
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0-b2
		pMat->FinalizeLayout();
		pMat->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create());
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["ShadowTest"] = pMat;
	}

	// DeferredLighting
	{
		auto pMat = new NXGraphicPassMaterial("DeferredLighting", L"Shader\\DeferredRender.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(9);  // t0-t8
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(5);  // b0-b4
		pMat->FinalizeLayout();
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["DeferredLighting"] = pMat;
	}

	// Subsurface
	{
		auto pMat = new NXGraphicPassMaterial("Subsurface", L"Shader\\SSSSSRenderer.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterDSV(DXGI_FORMAT_R24G8_TYPELESS);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(6);  // t0-t5
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(4);  // b0-b3
		pMat->FinalizeLayout();
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS, true, 0xFF, 0xFF, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		pMat->SetStencilRef(0x1);
		m_pPassMaterialMaps["Subsurface"] = pMat;
	}

	// SkyLighting
	{
		auto pMat = new NXGraphicPassMaterial("SkyLighting", L"Shader\\CubeMap.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterDSV(DXGI_FORMAT_R24G8_TYPELESS);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);  // t0
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);  // b0-b1
		pMat->FinalizeLayout();
		pMat->SetInputLayout(NXGlobalInputLayout::layoutP);
		pMat->SetRenderTargetMesh("_CubeMapSphere");
		pMat->SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["SkyLighting"] = pMat;
	}

	// PostProcessing
	{
		auto pMat = new NXGraphicPassMaterial("PostProcessing", L"Shader\\ColorMapping.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);  // t0
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0-b2
		pMat->FinalizeLayout();
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["PostProcessing"] = pMat;
	}

	// DebugLayer
	{
		auto pMat = new NXGraphicPassMaterial("DebugLayer", L"Shader\\DebugLayer.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);  // t0-t1
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0-b2
		pMat->FinalizeLayout();
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["DebugLayer"] = pMat;
	}

	// Gizmos
	{
		auto pMat = new NXGraphicPassMaterial("Gizmos", L"Shader\\EditorObjects.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0-b2
		pMat->FinalizeLayout();
		pMat->SetBlendState(NXBlendState<false, false, true, false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD>::Create());
		pMat->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE>::Create());
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS>::Create());
		pMat->SetInputLayout(NXGlobalInputLayout::layoutEditorObject);
		m_pPassMaterialMaps["Gizmos"] = pMat;
	}

	// Final
	{
		auto pMat = new NXGraphicPassMaterial("FinalQuad", L"Shader\\FinalQuad.fx");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R11G11B10_FLOAT });
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);  // t0
		pMat->FinalizeLayout();
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["FinalQuad"] = pMat;
	}

	for (auto& [name, mat] : m_pPassMaterialMaps)
	{
		if (name != "GBuffer")
			mat->Compile();
	}
}

void NXPassMaterialManager::Release()
{
	for (auto& [name, mat] : m_pPassMaterialMaps)
	{
		if (mat)
		{
			mat->Release();
			delete mat;
		}
	}
	m_pPassMaterialMaps.clear();
}

NXPassMaterial* NXPassMaterialManager::GetPassMaterial(const std::string& name)
{
	auto it = m_pPassMaterialMaps.find(name);
	if (it != m_pPassMaterialMaps.end())
		return it->second;
	return nullptr;
}
