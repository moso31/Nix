#include "NXPassMaterialManager.h"
#include "NXPassMaterial.h"
#include "NXRenderStates.h"
#include "NXGlobalDefinitions.h"

NXPassMaterialManager* NXPassMaterialManager::GetInstance()
{
	static NXPassMaterialManager instance;
	return &instance;
}

void NXPassMaterialManager::InitDefaultRenderer()
{
	////////////////////////////////////////
	// Virtual Texture
	////////////////////////////////////////
	
	// UpdateSector2IndirectTexture
	{
		auto pMat = new NXComputePassMaterial("UpdateSector2IndirectTexture", L"Shader\\VTSector2IndirectTexture.fx");
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(1);
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	// VT Physical Page
	{
		auto pMat = new NXComputePassMaterial("PhysicalPageBaker", L"Shader\\VTPhysicalPageBaker.fx");
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(4);
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);  // b0, b1, b2
		pMat->FinalizeLayout();
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		AddMaterial(pMat, true);
	}

	// VT Update IndirectTexture
	{
		auto pMat = new NXComputePassMaterial("UpdateIndirectTexture", L"Shader\\VTUpdateIndirectTexture.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);  // b0
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(11);  // u0
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	////////////////////////////////////////
	// GPU-Driven Terrain
	////////////////////////////////////////

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
		AddMaterial(pMat, true);
	}

	// Terrain Atlas Baker(normal, float4)
	{
		auto pMat = new NXComputePassMaterial("TerrainAtlasBaker:Float4", L"Shader\\TerrainAtlasBaker.fx");
		pMat->SetEntryNameCS(L"CS_Float4");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(4);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	// Terrain Sector2Node Tint
	{
		auto pMat = new NXComputePassMaterial("TerrainSector2NodeTint", L"Shader\\TerrainSector2NodeTint.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(6);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	// Terrain Nodes Culling
	{
		auto pMat = new NXComputePassMaterial("TerrainNodesCulling:First", L"Shader\\TerrainNodesCulling.fx");
		pMat->SetEntryNameCS(L"CS_First");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	// Terrain Nodes Culling
	{
		auto pMat = new NXComputePassMaterial("TerrainNodesCulling:Process", L"Shader\\TerrainNodesCulling.fx");
		pMat->SetEntryNameCS(L"CS_Process");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	// Terrain Patcher
	{
		auto pMat = new NXComputePassMaterial("TerrainPatcher", L"Shader\\TerrainPatcher.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(2);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
	}

	////////////////////////////////////////
	// Rendering
	////////////////////////////////////////

	// GBuffer
	{
		auto pMat = new NXGraphicPassMaterial("GBuffer");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM });
		pMat->RegisterDSV({ DXGI_FORMAT_R24G8_TYPELESS });
		pMat->RegisterSRVSpaceNum(3);
		pMat->RegisterSRVSlotNum(1, 2); // t0, space2
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(1, 0); // u0, space0
		pMat->FinalizeLayout();
		AddMaterial(pMat, false);
	}

	// VTReadback dataout
	{
		auto pMat = new NXReadbackPassMaterial("VTReadbackData");
		AddMaterial(pMat, true);
	}

	// ShadowMap
	{
		auto pMat = new NXGraphicPassMaterial("ShadowMap", L"Shader\\ShadowMap.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(3);
		pMat->RegisterDSV(DXGI_FORMAT_R32_TYPELESS);
		pMat->FinalizeLayout();
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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
		AddMaterial(pMat, true);
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

void NXPassMaterialManager::AddMaterial(NXPassMaterial* pMat, bool bCompile)
{
	std::string name = pMat->GetName();
	auto it = m_pPassMaterialMaps.find(name);
	if (it != m_pPassMaterialMaps.end())
	{
		// 如果已存在同名 PassMaterial，先注销旧的
		RemoveMaterial(name);
	}

	m_pPassMaterialMaps[name] = pMat;

	if (bCompile)
	{
		pMat->Compile();
	}
}

void NXPassMaterialManager::RemoveMaterial(const std::string& name)
{
	auto it = m_pPassMaterialMaps.find(name);
	if (it == m_pPassMaterialMaps.end())
		return;

	// 从主map中移除...
	NXPassMaterial* pMat = it->second;
	m_pPassMaterialMaps.erase(it);

	// ...但先放入待移除队列
	NXPassMaterialRemoving removing;
	removing.pMaterial = pMat;
	removing.fenceValue = NXGlobalDX::s_globalfenceValue;
	m_removingMaterials.push_back(removing);
}

void NXPassMaterialManager::FrameCleanup()
{
	UINT64 currentGPUFenceValue = NXGlobalDX::s_globalfence->GetCompletedValue();
	std::erase_if(m_removingMaterials, [currentGPUFenceValue](const NXPassMaterialRemoving& removing)
		{
			if (currentGPUFenceValue > removing.fenceValue)
			{
				removing.pMaterial->Release();
				delete removing.pMaterial;
				return true;
			}
			return false;
		});
}

NXPassMaterial* NXPassMaterialManager::GetPassMaterial(const std::string& name)
{
	auto it = m_pPassMaterialMaps.find(name);
	if (it != m_pPassMaterialMaps.end())
		return it->second;
	return nullptr;
}
