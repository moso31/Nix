#include "Renderer.h"
#include "NXTimer.h"
#include "NXGlobalBuffers.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "NXEvent.h"
#include "NXResourceManager.h"
#include "NXResourceReloader.h"
#include "NXRenderStates.h"
#include "NXGUI.h"
#include "NXTexture.h"
#include "NXBuffer.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXDepthPrepass.h"
#include "NXSimpleSSAO.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXPSOManager.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"
#include "NXGPUTerrainManager.h"
#include "NXVirtualTextureManager.h"
#include "NXTerrainStreamingBatcher.h"
#include "NXPassMaterial.h"
#include "NXPBRLight.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_pRenderGraph(nullptr),
	m_viewRTSize(rtSize),
	m_bEnablePostProcessing(true),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f),
	m_pNeedRebuildRenderGraph(true)
{
	m_cbDebugLayerData.LayerParam0 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
	m_vtReadbackData = new NXReadbackData("VT Readback CPUdata");
}

void Renderer::Init()
{
	// 输入事件
	InitEvents();

	// 初始化资源
	InitGlobalResources();

	// 全局通用纹理
	NXResourceManager::GetInstance()->GetTextureManager()->InitCommonTextures();

	NXSubMeshGeometryEditor::GetInstance()->Init(NXGlobalDX::GetDevice());

	m_scene = new NXScene();

	NXResourceManager::GetInstance()->GetMaterialManager()->Init();

	NXResourceManager::GetInstance()->GetMeshManager()->Init(m_scene);
	NXResourceManager::GetInstance()->GetCameraManager()->SetWorkingScene(m_scene);
	NXResourceManager::GetInstance()->GetLightManager()->SetWorkingScene(m_scene);

	NXGPUTerrainManager::GetInstance()->Init();

	m_scene->Init();
	m_pTerrainLODStreamer = new NXTerrainLODStreamer();
	m_pTerrainLODStreamer->Init(m_scene);
	NXVirtualTextureManager::GetInstance()->Init();
	NXVirtualTextureManager::GetInstance()->BuildSearchList(400);
	NXVirtualTextureManager::GetInstance()->SetCamera(m_scene->GetMainCamera());

	auto pCubeMap = m_scene->GetCubeMap();

	m_pBRDFLut = new NXBRDFLut();
	m_pBRDFLut->Init();

	m_pRenderGraph = new NXRenderGraph();

	InitPassMaterials();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	if (m_pRenderGraph)
		m_pRenderGraph->SetViewResolution(m_viewRTSize);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
	m_pGUI->SetVTReadbackData(m_vtReadbackData);
}

void Renderer::InitPassMaterials()
{
	// TerrainFillTest
	{
		auto pMat = new NXComputePassMaterial("TerrainFillTest", L"Shader\\FillTestComputeShader.fx");
		pMat->SetEntryNameCS(L"CS_Pass");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["TerrainFillTest"] = pMat;
	}

	// TerrainGPUPatcher Clear
	{
		auto pMat = new NXComputePassMaterial("TerrainGPUPatcher", L"Shader\\GPUTerrainPatcher.fx");
		pMat->SetEntryNameCS(L"CS_Clear");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);
		pMat->FinalizeLayout();
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["TerrainGPUPatcher:clear"] = pMat;
	}

	// TerrainGPUPatcher Patch
	{
		auto pMat = new NXComputePassMaterial("TerrainGPUPatcher", L"Shader\\GPUTerrainPatcher.fx");
		pMat->SetEntryNameCS(L"CS_Patch");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(2);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(2);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(3);
		pMat->FinalizeLayout();
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["TerrainGPUPatcher:patch"] = pMat;
	}

	// GBuffer（GBuffer由Mesh提供CBV SRV，这里不需要）
	{
		auto pMat = new NXGraphicPassMaterial("GBuffer");
		pMat->RegisterRTVNum({ DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM });
		pMat->RegisterDSV({ DXGI_FORMAT_R24G8_TYPELESS });
		m_pPassMaterialMaps["GBuffer"] = pMat;
	}

	// VTReadback computeshader
	{
		auto pMat = new NXComputePassMaterial("VTReadback", L"Shader\\VTReadback.fx");
		pMat->RegisterCBVSpaceNum(1);
		pMat->RegisterCBVSlotNum(1);
		pMat->RegisterSRVSpaceNum(1);
		pMat->RegisterSRVSlotNum(1);
		pMat->RegisterUAVSpaceNum(1);
		pMat->RegisterUAVSlotNum(1);
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
		pMat->RegisterDSV(DXGI_FORMAT_D24_UNORM_S8_UINT);
		pMat->FinalizeLayout();
		m_pPassMaterialMaps["ShadowMap"] = pMat;
	}

	// ShadowTest
	{
		auto pMat = new NXGraphicPassMaterial("ShadowTest", L"Shader\\ShadowTest.fx");
		pMat->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, 0, 0, 1000.0f>::Create());
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["ShadowTest"] = pMat;
	}

	// DeferredLighting
	{
		auto pMat = new NXGraphicPassMaterial("DeferredLighting", L"Shader\\DeferredRender.fx");
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["DeferredLighting"] = pMat;
	}

	// Subsurface
	{
		auto pMat = new NXGraphicPassMaterial("Subsurface", L"Shader\\SSSSSRenderer.fx");
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS, true, 0xFF, 0xFF, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		pMat->SetStencilRef(0x1);
		m_pPassMaterialMaps["Subsurface"] = pMat;
	}

	// SkyLighting
	{
		auto pMat = new NXGraphicPassMaterial("SkyLighting", L"Shader\\CubeMap.fx");
		pMat->SetInputLayout(NXGlobalInputLayout::layoutP);
		pMat->SetRenderTargetMesh("_CubeMapSphere");
		pMat->SetDepthStencilState(NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["SkyLighting"] = pMat;
	}

	// PostProcessing
	{
		auto pMat = new NXGraphicPassMaterial("PostProcessing", L"Shader\\ColorMapping.fx");
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		m_pPassMaterialMaps["PostProcessing"] = pMat;
	}

	// DebugLayer
	{
		auto pMat = new NXGraphicPassMaterial("DebugLayer", L"Shader\\DebugLayer.fx");
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_ALWAYS>::Create());
		pMat->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		m_pPassMaterialMaps["DebugLayer"] = pMat;
	}

	// Gizmos
	{
		auto pMat = new NXGraphicPassMaterial("Gizmos", L"Shader\\EditorObjects.fx");
		pMat->SetBlendState(NXBlendState<false, false, true, false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD>::Create());
		pMat->SetRasterizerState(NXRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE>::Create());
		pMat->SetDepthStencilState(NXDepthStencilState<false, false, D3D12_COMPARISON_FUNC_LESS>::Create());
		pMat->SetInputLayout(NXGlobalInputLayout::layoutEditorObject);
		m_pPassMaterialMaps["Gizmos"] = pMat;
	}
}

void Renderer::GenerateRenderGraph()
{
	m_pRenderGraph->Destroy();
	m_pRenderGraph->SetViewResolution(m_viewRTSize);

	//struct TerrainStreamBatcherData
	//{

	//};

	//m_pRenderGraph->AddComputePass<TerrainStreamBatcherData>("Terrain Streaming Batcher", new NXTerrainStreamingPass(),
	//	[=](NXRGBuilder& builder, TerrainStreamBatcherData& data) {
	//		builder.SetSubmitGroup(0);
	//		//builder.Read()
	//	},
	//	[=](ID3D12GraphicsCommandList* pCmdList, TerrainStreamBatcherData& data) {
	//	});

	auto terrIns = NXGPUTerrainManager::GetInstance();
	NXRGHandle hTerrainBufferA			= m_pRenderGraph->Import(terrIns->GetTerrainBufferA());
	NXRGHandle hTerrainBufferB			= m_pRenderGraph->Import(terrIns->GetTerrainBufferB());
	NXRGHandle hTerrainBufferFinal		= m_pRenderGraph->Import(terrIns->GetTerrainFinalBuffer());
	NXRGHandle hTerrainIndiArgs			= m_pRenderGraph->Import(terrIns->GetTerrainIndirectArgs());
	NXRGHandle hTerrainPatcher			= m_pRenderGraph->Import(terrIns->GetTerrainPatcherBuffer());
	NXRGHandle hTerrainDrawIndexArgs	= m_pRenderGraph->Import(terrIns->GetTerrainDrawIndexArgs());

	struct FillTestData
	{
	};

	for (int i = 0; i < 6; i++)
	{
		NXRGHandle hInput = i % 2 ? hTerrainBufferB : hTerrainBufferA;
		NXRGHandle hOutput = i % 2 ? hTerrainBufferA : hTerrainBufferB;

		std::string strBufName = "Terrain Fill " + std::to_string(i);
		m_pRenderGraph->AddComputePass<FillTestData>(strBufName,
			[=](NXRGBuilder& builder, FillTestData& data) {
				builder.Write(hInput);
				builder.Write(hOutput);
				builder.Write(hTerrainBufferFinal);
			},
			[=](ID3D12GraphicsCommandList* pCmdList, FillTestData& data) mutable {
				auto pMat = static_cast<NXComputePassMaterial*>(m_pPassMaterialMaps["TerrainFillTest"]);
				pMat->SetOutput(0, 0, pInput);
				pMat->SetOutput(0, 1, pOutput);
				pMat->SetOutput(0, 2, pTerrainBufferFinal);
				pMat->SetConstantBuffer(0, 0, &NXGPUTerrainManager::GetInstance()->GetCBTerrainParams(i));

				if (i == 0)
				{
					std::vector<NXGPUTerrainBlockData> initData; // NXGPUTerrainBlockData = Int2
					int step = 4;
					for (int x = -step; x < step; x++)
					{
						for (int y = -step; y < step; y++)
						{
							initData.push_back({ x, y });
						}
					}

					pInput->SetCurrent(initData.data(), initData.size());
					pTerrainBufferFinal->SetCurrent(nullptr, 0);
				}
				NXGPUTerrainManager::GetInstance()->UpdateLodParams(i);

				// 拷贝pInput.UAV计数器 作为 dispatch indirect args
				// 虽然只是拷贝pInput.UAV计数器，但目前的设计不太灵活，要SetResourceState必须带着原始资源一起做...
				pInput->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
				pTerrainIndiArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
				// ...不过最终拷贝的时候，只拷贝pInput.UAV计数器即可。
				pCmdList->CopyBufferRegion(pTerrainIndiArgs->GetD3DResource(), 0, pInput->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);

				pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pTerrainIndiArgs->GetD3DResource(), 0, nullptr, 0);
			});
	}

	struct GPUTerrainPatcherData 
	{
	};

	m_pRenderGraph->AddComputePass<GPUTerrainPatcherData>("GPU Terrain Patcher Clear",
		[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
			builder.Write(hTerrainPatcher);
			builder.Write(hTerrainDrawIndexArgs);
			builder.Write(hTerrainPatcher);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, GPUTerrainPatcherData& data) {
			auto pMat = static_cast<NXComputePassMaterial*>(m_pPassMaterialMaps["TerrainGPUPatcher:clear"]);
			pMat->SetOutput(0, 0, pTerrainPatcher);
			pMat->SetOutput(0, 1, pTerrainDrawIndexArgs);
			pMat->SetOutput(0, 2, pTerrainPatcher, true);
			pMat->SetThreadGroups(1, 1, 1);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	auto hTerrain_MinMaxZMap2DArray = m_pRenderGraph->Import(terrIns->GetTerrainMinMaxZMap2DArray());

	m_pRenderGraph->AddComputePass<GPUTerrainPatcherData>("GPU Terrain Patcher",
		[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
			builder.Read(hTerrain_MinMaxZMap2DArray);
			builder.Read(hTerrainBufferFinal);
			builder.Write(hTerrainPatcher);
			builder.Write(hTerrainDrawIndexArgs);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, GPUTerrainPatcherData& data) mutable {
			auto pMat = static_cast<NXComputePassMaterial*>(m_pPassMaterialMaps["TerrainGPUPatcher:patch"]);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &NXGPUTerrainManager::GetInstance()->GetTerrainSupportParam());
			pMat->SetInput(0, 0, pTerrain_MinMaxZMap2DArray->GetTexture());
			pMat->SetInput(0, 1, pTerrainBufferFinal);
			pMat->SetOutput(0, 0, pTerrainPatcher);
			pMat->SetOutput(0, 1, pTerrainDrawIndexArgs);
			pMat->SetIndirectArguments(pTerrainIndiArgs);

			// 虽然只是拷贝pInput.UAV计数器，但目前的设计不太灵活，要SetResourceState必须带着原始资源一起做...
			pTerrainBufferFinal->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pTerrainIndiArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
			// ...不过最终拷贝的时候，只拷贝pInput.UAV计数器即可。
			pCmdList->CopyBufferRegion(pTerrainIndiArgs->GetD3DResource(), 0, pTerrainBufferFinal->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	NXRGHandle hGBuffer0 = m_pRenderGraph->Create("GBuffer RT0", { .format = DXGI_FORMAT_R32_FLOAT, .type = NXResourceType::Texture2D });
	NXRGHandle hGBuffer1 = m_pRenderGraph->Create("GBuffer RT1", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .type = NXResourceType::Texture2D });
	NXRGHandle hGBuffer2 = m_pRenderGraph->Create("GBuffer RT2", { .format = DXGI_FORMAT_R10G10B10A2_UNORM, .type = NXResourceType::Texture2D });
	NXRGHandle hGBuffer3 = m_pRenderGraph->Create("GBuffer RT3", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .type = NXResourceType::Texture2D });
	NXRGHandle hDepthZ = m_pRenderGraph->Create("DepthZ", { .format = DXGI_FORMAT_R24G8_TYPELESS, .type = NXResourceType::Texture2D });

	struct GBufferData
	{
		NXRGHandle depth;
		NXRGHandle rt0;
		NXRGHandle rt1;
		NXRGHandle rt2;
		NXRGHandle rt3;
	};

	auto gBufferPassData = m_pRenderGraph->AddPass<GBufferData>("GBufferPass",
		[&](NXRGBuilder& builder, GBufferData& data) {
			data.rt0	= builder.Write(hGBuffer0);
			data.rt1	= builder.Write(hGBuffer1);
			data.rt2	= builder.Write(hGBuffer2);
			data.rt3	= builder.Write(hGBuffer3);
			data.depth	= builder.Write(hDepthZ);
		}, 
		[=](ID3D12GraphicsCommandList* pCmdList, GBufferData& data) {
			Ntr<NXTexture> pOutRTs[] = {
				data.rt0->GetTexture(),
				data.rt1->GetTexture(),
				data.rt2->GetTexture(),
				data.rt3->GetTexture()
			};
			auto pOutDS = data.depth->GetTexture();

			auto* pPassMaterial = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["GBuffer"]);
			for (int i = 0; i < 4; i++)
			{
				pOutRTs[i]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				pCmdList->ClearRenderTargetView(pOutRTs[i]->GetRTV(), Colors::Black, 0, nullptr);
				pPassMaterial->SetOutputRT(i, pOutRTs[i]);
			}
			pOutDS->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			pCmdList->ClearDepthStencilView(pOutDS->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0x0, 0, nullptr);
			pPassMaterial->SetOutputDS(pOutDS);

			auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
			pCmdList->RSSetViewports(1, &vpCamera);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));

			pPassMaterial->RenderSetTargetAndState(pCmdList);

			auto pErrorMat = NXResourceManager::GetInstance()->GetMaterialManager()->GetErrorMaterial();
			auto pMaterialsArray = NXResourceManager::GetInstance()->GetMaterialManager()->GetMaterials();
			for (auto pMat : pMaterialsArray)
			{
				auto pCustomMat = pMat ? pMat->IsCustomMat() : nullptr;
				pCmdList->OMSetStencilRef(0x0);

				if (pMat)
				{
					// 更新材质CB
					pMat->Update();
				}

				if (pCustomMat && pCustomMat->GetCompileSuccess())
				{
					if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
					{
						// 3S材质需要写模板缓存
						pCmdList->OMSetStencilRef(0x1);
					}
				}

				pMat->Render(pCmdList);
				m_scene->GetMainCamera()->Render(pCmdList);
				for (auto pSubMesh : pMat->GetRefSubMeshes())
				{
					if (pSubMesh)
					{
						bool bIsVisible = pSubMesh->GetRenderableObject()->GetVisible();
						if (bIsVisible)
						{
							if (pSubMesh->IsSubMeshTerrain())
							{
								NXGPUTerrainManager::GetInstance()->UpdateConstantForGBuffer(pCmdList);
								pSubMesh->Render(pCmdList);
								break;
							}

							pSubMesh->GetRenderableObject()->Update(pCmdList); // 永远优先调用派生类的Update 
							pSubMesh->Render(pCmdList);
						}
					}
				}
			}
		});

	NXRGHandle pVTReadback = m_pRenderGraph->Create("VT Readback Buffer", { .isViewRT = true, .RTScale = 0.125f, .type = NXResourceType::Buffer, .format = DXGI_FORMAT_R32_FLOAT });
	struct VTReadback
	{
	};
	m_pRenderGraph->AddComputePass<VTReadback>("VTReadbackPass",
		[&](NXRGBuilder& builder, VTReadback& data) {
			builder.Read(gBufferPassData->GetData().rt0);
			builder.Write(pVTReadback);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, VTReadback& data) {
			auto pMat = static_cast<NXComputePassMaterial*>(m_pPassMaterialMaps["VTReadback"]);
			pMat->SetConstantBuffer(0, 0, &NXVirtualTextureManager::GetInstance()->GetCBufferVTReadback());
			pMat->SetInput(0, 0, pGBuffer0->GetTexture());
			pMat->SetOutput(0, 0, pVTReadback->GetBuffer());

			auto& pRT = pGBuffer0->GetTexture();
			Int2 rtSize(pRT->GetWidth(), pRT->GetHeight());
			Int2 threadGroupSize((rtSize + 7) / 8);
			pMat->SetThreadGroups(threadGroupSize.x, threadGroupSize.y);

			// 记录VTReadback的size 
			m_vtReadbackDataSize = threadGroupSize;
			m_pGUI->SetVTReadbackDataSize(threadGroupSize);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	// TODO: 简略变量，new NXReadbackBufferPass有必要吗？
	struct VTReadbackData
	{
	};
	m_pRenderGraph->AddReadbackBufferPass<VTReadbackData>("",
		[&](NXRGBuilder& builder, VTReadbackData& data) {
			builder.Read(pVTReadback);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, VTReadbackData& data) {
			auto pMat = static_cast<NXReadbackPassMaterial*>(m_pPassMaterialMaps["VTReadbackData"]);
			pMat->SetInput(pVTReadback->GetBuffer());
			pMat->SetOutput(m_vtReadbackData);
			pMat->Render(pCmdList);
		});

	struct ShadowMapData
	{
	};

	NXRGHandle pCSMDepth = m_pRenderGraph->Import(m_pTexCSMDepth);
	auto shadowMapPassData = m_pRenderGraph->AddPass<ShadowMapData>("ShadowMap",
		[&](NXRGBuilder& builder, ShadowMapData& data) {
			builder.Write(pCSMDepth);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, ShadowMapData& data) {
			auto vpCamera = NX12Util::ViewPort((float)m_shadowMapRTSize, (float)m_shadowMapRTSize);
			pCmdList->RSSetViewports(1, &vpCamera);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));

			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["ShadowMap"]);
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
			pMat->SetOutputDS(pCSMDepth->GetTexture());
			pMat->FinalizeLayout();

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			for (auto pLight : m_scene->GetPBRLights())
			{
				if (pLight->GetType() == NXLightTypeEnum::NXLight_Distant)
				{
					NXPBRDistantLight* pDirLight = static_cast<NXPBRDistantLight*>(pLight);
					RenderCSMPerLight(pCmdList, pDirLight);
				}
			}
		});

	struct ShadowTestData
	{
		NXRGHandle shadowTest;
	};

	NXRGHandle pShadowTest = m_pRenderGraph->Create("ShadowTest RT", { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .handleFlags = RG_RenderTarget });

	auto shadowTestPassData = m_pRenderGraph->AddPass<ShadowTestData>("ShadowTest",
		[&](NXRGBuilder& builder, ShadowTestData& data) {
			builder.Read(gBufferPassData->GetData().depth);
			builder.Read(pCSMDepth);
			data.shadowTest = builder.Write(pShadowTest);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, ShadowTestData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["ShadowTest"]);
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
			pMat->SetInputTex(0, 0, gBufferPassData->GetData().depth->GetTexture());
			pMat->SetInputTex(0, 1, pCSMDepth->GetTexture());
			pMat->SetOutputRT(0, pShadowTest->GetTexture());

			m_pRenderGraph->SetViewPortAndScissorRect(pCmdList, m_viewRTSize);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct DeferredLightingData
	{
		NXRGHandle lighting;
		NXRGHandle lightingSpec;
		NXRGHandle lightingCopy;
	};

	NXRGHandle pLit			= m_pRenderGraph->Create("Lighting RT0", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGHandle pLitSpec		= m_pRenderGraph->Create("Lighting RT1", { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGHandle pLitCopy		= m_pRenderGraph->Create("Lighting RT Copy", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });
	NXRGHandle pCubeMap		= m_pRenderGraph->Import(m_scene->GetCubeMap()->GetCubeMap());
	NXRGHandle pPreFilter	= m_pRenderGraph->Import(m_scene->GetCubeMap()->GetPreFilterMap());
	NXRGHandle pBRDFLut		= m_pRenderGraph->Import(m_pBRDFLut->GetTex());

	auto litPassData = m_pRenderGraph->AddPass<DeferredLightingData>("DeferredLighting",
		[&](NXRGBuilder& builder, DeferredLightingData& data) {
			builder.Read(gBufferPassData->GetData().rt0);
			builder.Read(gBufferPassData->GetData().rt1);
			builder.Read(gBufferPassData->GetData().rt2);
			builder.Read(gBufferPassData->GetData().rt3);
			builder.Read(gBufferPassData->GetData().depth);
			builder.Read(shadowTestPassData->GetData().shadowTest);
			builder.Read(pCubeMap);
			builder.Read(pPreFilter);
			builder.Read(pBRDFLut);
			data.lighting = builder.Write(pLit); 
			data.lightingSpec = builder.Write(pLitSpec); 
			data.lightingCopy = builder.Write(pLitCopy); 
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DeferredLightingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["DeferredLighting"]);
			pMat->SetConstantBuffer(0, 0, &g_cbObject); // b0
			pMat->SetConstantBuffer(0, 1, &g_cbCamera); // b1
			pMat->SetConstantBuffer(0, 2, &m_scene->GetConstantBufferLights()); // b2
			pMat->SetConstantBuffer(0, 3, &m_scene->GetCubeMap()->GetCBDataParams()); // b3
			pMat->SetConstantBuffer(0, 4, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile()); // b4

			pMat->SetInputTex(0, 0, gBufferPassData->GetData().rt0->GetTexture()); // register t0
			pMat->SetInputTex(0, 1, gBufferPassData->GetData().rt1->GetTexture()); // t1
			pMat->SetInputTex(0, 2, gBufferPassData->GetData().rt2->GetTexture()); // t2
			pMat->SetInputTex(0, 3, gBufferPassData->GetData().rt3->GetTexture()); // t3
			pMat->SetInputTex(0, 4, gBufferPassData->GetData().depth->GetTexture()); // t4
			pMat->SetInputTex(0, 5, shadowTestPassData->GetData().shadowTest->GetTexture()); // t5
			pMat->SetInputTex(0, 6, pCubeMap->GetTexture());
			pMat->SetInputTex(0, 7, pPreFilter->GetTexture());
			pMat->SetInputTex(0, 8, pBRDFLut->GetTexture());

			pMat->SetOutputRT(0, data.lighting->GetTexture()); 
			pMat->SetOutputRT(1, data.lightingSpec->GetTexture()); 
			pMat->SetOutputRT(2, data.lightingCopy->GetTexture()); 

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct SubsurfaceData
	{
		NXRGHandle buf;
		NXRGHandle depth;
	};
	NXRGHandle pNoise64 = m_pRenderGraph->Import(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64));
	auto sssPassData = m_pRenderGraph->AddPass<SubsurfaceData>("Subsurface",
		[&](NXRGBuilder& builder, SubsurfaceData& data) {
			builder.Read(litPassData->GetData().lighting);
			builder.Read(litPassData->GetData().lightingSpec);
			builder.Read(litPassData->GetData().lightingCopy);
			builder.Read(gBufferPassData->GetData().rt1);
			builder.Read(gBufferPassData->GetData().depth);
			builder.Read(pNoise64);

			data.buf	= builder.Write(litPassData->GetData().lighting);
			data.depth	= builder.Write(gBufferPassData->GetData().depth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, SubsurfaceData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["Subsurface"]);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 3, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

			pMat->SetInputTex(0, 0, litPassData->GetData().lighting->GetTexture());
			pMat->SetInputTex(0, 1, litPassData->GetData().lightingSpec->GetTexture());
			pMat->SetInputTex(0, 2, litPassData->GetData().lightingCopy->GetTexture());
			pMat->SetInputTex(0, 3, gBufferPassData->GetData().rt1->GetTexture());
			pMat->SetInputTex(0, 4, gBufferPassData->GetData().depth->GetTexture());
			pMat->SetInputTex(0, 5, pNoise64->GetTexture());

			pMat->SetOutputRT(0, data.buf->GetTexture());
			pMat->SetOutputDS(data.depth->GetTexture());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct SkyLightingData
	{
		NXRGHandle buf;
		NXRGHandle depth;
	};
	auto skyPassData = m_pRenderGraph->AddPass<SkyLightingData>("SkyLighting",
		[&](NXRGBuilder& builder, SkyLightingData& data) {
			builder.Read(pCubeMap);
			data.buf	= builder.Write(sssPassData->GetData().buf);
			data.depth	= builder.Write(gBufferPassData->GetData().depth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, SkyLightingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["SkyLighting"]);
			pMat->SetConstantBuffer(0, 0, &m_scene->GetCubeMap()->GetCBObjectParams());
			pMat->SetConstantBuffer(0, 1, &m_scene->GetCubeMap()->GetCBDataParams());
			pMat->SetInputTex(0, 0, pCubeMap->GetTexture());

			pMat->SetOutputRT(0, data.buf->GetTexture());
			pMat->SetOutputDS(data.depth->GetTexture());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct PostProcessingData
	{
		NXRGHandle out;
	};

	NXRGHandle pPostProcess = m_pRenderGraph->Create("PostProcessing RT", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });

	auto postProcessPassData = m_pRenderGraph->AddPass<PostProcessingData>("PostProcessing",
		[&](NXRGBuilder& builder, PostProcessingData& data) {
			builder.Read(skyPassData->GetData().buf);
			data.out = builder.Write(pPostProcess);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, PostProcessingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["PostProcessing"]);
			
			m_cbColorMappingData.param0.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
			m_cbColorMapping.Update(m_cbColorMappingData);
			
			pMat->SetConstantBuffer(0, 2, &m_cbColorMapping);
			pMat->SetInputTex(0, 0, skyPassData->GetData().buf->GetTexture());
			pMat->SetOutputRT(0, data.out->GetTexture());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct DebugLayerData
	{
		NXRGHandle out;
	};

	NXRGHandle pDebugLayer = m_pRenderGraph->Create("Debug Layer RT", { .format = DXGI_FORMAT_R11G11B10_FLOAT, .handleFlags = RG_RenderTarget });

	auto debugLayerPassData = m_pRenderGraph->AddPass<DebugLayerData>("DebugLayer",
		[&](NXRGBuilder& builder, DebugLayerData& data) {
			builder.Read(postProcessPassData->GetData().out);
			builder.Read(pCSMDepth);
			data.out = builder.Write(pDebugLayer);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, DebugLayerData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["DebugLayer"]);
			
			m_cbDebugLayerData.LayerParam0.x = (float)m_bEnableShadowMapDebugLayer;
			m_cbDebugLayerData.LayerParam0.y = m_fShadowMapZoomScale;
			m_cbDebugLayer.Update(m_cbDebugLayerData);
			
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &m_cbDebugLayer);
			pMat->SetInputTex(0, 0, postProcessPassData->GetData().out->GetTexture());
			pMat->SetInputTex(0, 1, pCSMDepth->GetTexture());
			pMat->SetOutputRT(0, data.out->GetTexture());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
		});

	struct GizmosData
	{
		NXRGHandle out;
	};
	auto gizmosPassData = m_pRenderGraph->AddPass<GizmosData>("Gizmos",
		[&](NXRGBuilder& builder, GizmosData& data) {
			NXRGHandle pOut = m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out;
			data.out = builder.Write(pOut);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, GizmosData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["Gizmos"]);
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, nullptr);
			pMat->SetOutputRT(0, data.out->GetTexture());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			NXEditorObjectManager* pEditorObjManager = m_scene->GetEditorObjManager();
			for (auto pEditObj : pEditorObjManager->GetEditableObjects())
			{
				if (pEditObj->GetVisible()) // if bIsVisible
				{
					pEditObj->Update(pCmdList); // b0 update in here.

					for (UINT i = 0; i < pEditObj->GetSubMeshCount(); i++)
					{
						auto pSubMesh = pEditObj->GetSubMesh(i);
						if (pSubMesh->IsSubMeshEditorObject())
						{
							NXSubMeshEditorObjects* pSubMeshEditorObj = (NXSubMeshEditorObjects*)pSubMesh;
							bool bIsHighLight = pSubMeshEditorObj->GetEditorObjectID() == m_scene->GetEditorObjManager()->GetHighLightID();
							pSubMeshEditorObj->Update(pCmdList, bIsHighLight);
							pSubMeshEditorObj->Render(pCmdList);
						}
					}
				}
			}
		});

	m_pRenderGraph->SetPresent(&gizmosPassData->GetData().out);
}
 
void Renderer::NotifyRebuildRenderGraph()
{
	m_pNeedRebuildRenderGraph = true;
}

void Renderer::InitEvents()
{
	NXEventKeyDown::GetInstance()->AddListener(std::bind(&Renderer::OnKeyDown, this, std::placeholders::_1));
}

void Renderer::InitGlobalResources()
{
	// GlobalInputLayout
	NXGlobalInputLayout::Init();

	// indirectArgs
	D3D12_INDIRECT_ARGUMENT_DESC indirectArgDesc = {};
	indirectArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	desc.pArgumentDescs = &indirectArgDesc;
	desc.NumArgumentDescs = 1;
	desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
	desc.NodeMask = 0;
	m_pCommandSig = NX12Util::CreateCommandSignature(NXGlobalDX::GetDevice(), desc, nullptr);

	// shadow map
	m_pTexCSMDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture2DArray("Shadow DepthZ RT", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false);
	m_pTexCSMDepth->SetViews(1, 0, m_cascadeCount, 0);
	for (UINT i = 0; i < m_cascadeCount; i++) m_pTexCSMDepth->SetDSV(i, i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pTexCSMDepth->SetSRV(0, 0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）
}

void Renderer::ResourcesReloading(DirectResources* pDXRes)
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();

	m_pRenderGraph->Clear();
	GenerateRenderGraph();

	//if (m_pNeedRebuildRenderGraph)
	//{
	//	pDXRes->Flush();
	//	GenerateRenderGraph();
	//	m_pNeedRebuildRenderGraph = false;
	//}

	// 每帧都Compile
	m_pRenderGraph->Setup();
	m_pRenderGraph->Compile();
}

void Renderer::Update()
{
	m_pTerrainLODStreamer->ProcessCompletedStreamingTask();
	m_pTerrainLODStreamer->Update();

	UpdateGUI();
	UpdateSceneData();
}

void Renderer::UpdateGUI()
{
	m_pGUI->ExecuteDeferredCommands();
}

void Renderer::UpdateSceneData()
{
	UpdateTime();

	// 更新场景Scripts。实际上是用Scripts控制指定物体的Transform。
	m_scene->UpdateScripts();

	// 更新Transform
	m_scene->UpdateTransform();
	m_scene->UpdateTransformOfEditorObjects();

	// 更新Camera的常量缓存数据（VP矩阵、眼睛位置）
	m_scene->UpdateCamera();

	auto* pCamera = m_scene->GetMainCamera();
	NXGPUTerrainManager::GetInstance()->UpdateCameraParams(pCamera);
	NXVirtualTextureManager::GetInstance()->Update();
	NXVirtualTextureManager::GetInstance()->UpdateCBData(pCamera->GetRTSize());

	m_scene->UpdateLightData();

	auto pCubeMap = m_scene->GetCubeMap();
	if (pCubeMap)
	{
		pCubeMap->Update(nullptr);
	}

	//m_pSSAO->Update();
}

void Renderer::UpdateTime()
{
	g_cbDataObject.globalData.time = NXGlobalApp::Timer->GetGlobalTimeSeconds();
}

void Renderer::RenderFrame()
{
	// 确保BRDF 2D LUT 异步加载完成
	m_pBRDFLut->WaitTexLoadFinish();

	// 执行RenderGraph!
	m_pRenderGraph->Execute();

	// 更新PSOManager状态
	NXPSOManager::GetInstance()->FrameCleanup();
}

void Renderer::RenderGUI(const NXSwapChainBuffer& swapChainBuffer)
{
	if (m_bRenderGUI)
	{
		auto backbuffer = m_pRenderGraph->GetPresent();
		if (backbuffer.IsValid())
			m_pGUI->Render(backbuffer, swapChainBuffer);
	}
}

void Renderer::Release()
{
	m_pRenderGraph->Destroy();

	SafeRelease(m_pGUI);
	SafeRelease(m_pBRDFLut);
	SafeRelease(m_scene);

	NXVirtualTextureManager::GetInstance()->Release();
	NXAllocatorManager::GetInstance()->Release();
	NXSubMeshGeometryEditor::GetInstance()->Release();
}

void Renderer::OnKeyDown(NXEventArgKey eArg)
{
	//if (eArg.VKey == 'H')
	//{
	//	m_bRenderGUI = !m_bRenderGUI;
	//}
}

void Renderer::RenderCSMPerLight(ID3D12GraphicsCommandList* pCmdList, NXPBRDistantLight* pDirLight)
{
	// 设置test_transition参数
	g_cbDataShadowTest.test_transition = 1.0f; // 默认值

	Vector3 lightDirection = pDirLight->GetDirection();
	lightDirection = lightDirection.IsZero() ? Vector3(0.0f, 0.0f, 1.0f) : lightDirection;
	lightDirection.Normalize();

	NXCamera* pCamera = m_scene->GetMainCamera();
	Vector3 cameraPosition = pCamera->GetTranslation();
	Vector3 cameraDirection = pCamera->GetForward();

	Matrix mxCamViewProjInv = pCamera->GetViewProjectionInverseMatrix();
	Matrix mxCamProjInv = pCamera->GetProjectionInverseMatrix();

	float zNear = pCamera->GetZNear();
	float shadowDistance = g_cbDataShadowTest.shadowDistance;
	float zLength = shadowDistance - zNear;

	Matrix mxCamProj = pCamera->GetProjectionMatrix();

	UINT cascadeCount = g_cbDataShadowTest.cascadeCount;
	float cascadeExponentScale = 2.5f; // 常用值
	float cascadeTransitionScale = g_cbDataShadowTest.cascadeTransitionScale;
	uint32_t shadowMapRTSize = 2048;

	float expScale = 1.0f;
	float sumInv = 1.0f;
	for (UINT i = 1; i < cascadeCount; i++)
	{
		expScale *= cascadeExponentScale;
		sumInv += expScale;
	}
	sumInv = 1.0f / sumInv;

	expScale = 1.0f;
	float percentage = 0.0f;
	float zLastCascadeTransitionLength = 0.0f;
	for (UINT i = 0; i < cascadeCount; i++)
	{
		// 按等比数列划分 cascade
		float percentageOffset = expScale * sumInv;
		expScale *= cascadeExponentScale;

		float zCascadeNear = zNear + percentage * zLength;
		percentage += percentageOffset;
		float zCascadeFar = zNear + percentage * zLength;
		float zCascadeLength = zCascadeFar - zCascadeNear;

		zCascadeNear -= zLastCascadeTransitionLength;

		// 此数值 用于 cascade 之间的平滑过渡
		zLastCascadeTransitionLength = zCascadeLength * cascadeTransitionScale;

		zCascadeLength += zLastCascadeTransitionLength;

		g_cbDataShadowTest.frustumParams[i] = Vector4(zCascadeFar, zLastCascadeTransitionLength, 0.0f, 0.0f);

		float zCascadeNearProj = (zCascadeNear * mxCamProj._33 + mxCamProj._43) / zCascadeNear;
		float zCascadeFarProj = (zCascadeFar * mxCamProj._33 + mxCamProj._43) / zCascadeFar;

		// 计算各层 cascade 的 Frustum (view space)
		Vector3 viewFrustum[8];
		viewFrustum[0] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[1] = Vector3::Transform(Vector3(-1.0f, 1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[2] = Vector3::Transform(Vector3(1.0f, -1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[3] = Vector3::Transform(Vector3(1.0f, 1.0f, zCascadeNearProj), mxCamProjInv);
		viewFrustum[4] = Vector3::Transform(Vector3(-1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[5] = Vector3::Transform(Vector3(-1.0f, 1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[6] = Vector3::Transform(Vector3(1.0f, -1.0f, zCascadeFarProj), mxCamProjInv);
		viewFrustum[7] = Vector3::Transform(Vector3(1.0f, 1.0f, zCascadeFarProj), mxCamProjInv);

		// 计算 Frustum 的外接球
		float a2 = (viewFrustum[3] - viewFrustum[0]).LengthSquared();
		float b2 = (viewFrustum[7] - viewFrustum[4]).LengthSquared();
		float delta = zCascadeLength * 0.5f + (a2 - b2) / (8.0f * zCascadeLength);

		// 计算 外接球 的 球心，view space 和 world space 都要。
		// zCascadeDistance: 当前 cascade 中 Near平面中心点 到 frustum 外接球心 的距离
		float zCascadeDistance = zCascadeLength - delta;
		Vector3 sphereCenterVS = Vector3(0.0f, 0.0f, zCascadeNear + zCascadeDistance);
		Vector3 sphereCenterWS = cameraPosition + cameraDirection * sphereCenterVS.z;

		// 计算 外接球 的 半径
		float sphereRadius = sqrtf(zCascadeDistance * zCascadeDistance + (a2 * 0.25f));

		Vector3 shadowMapEye = Vector3(0.0f);
		Vector3 shadowMapAt = -lightDirection;
		Vector3 shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		Matrix mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		float cascadeBound = sphereRadius * 2.0f;
		float worldUnitsPerPixel = cascadeBound / shadowMapRTSize;

		// "LS" = "light space" = shadow camera ortho space.
		Vector3 sphereCenterLS = Vector3::Transform(sphereCenterWS, mxShadowView);
		sphereCenterLS -= Vector3(fmodf(sphereCenterLS.x, worldUnitsPerPixel), fmodf(sphereCenterLS.y, worldUnitsPerPixel), 0.0f);
		sphereCenterWS = Vector3::Transform(sphereCenterLS, mxShadowView.Invert());

		Vector3 sceneCenter = m_scene->GetBoundingSphere().Center;
		float sceneRadius = m_scene->GetBoundingSphere().Radius;
		float backDistance = Vector3::Distance(sceneCenter, sphereCenterWS) + sphereRadius;
		shadowMapEye = sphereCenterWS - lightDirection * backDistance;
		shadowMapAt = sphereCenterWS;
		shadowMapUp = Vector3(0.0f, 1.0f, 0.0f);
		mxShadowView = XMMatrixLookAtLH(shadowMapEye, shadowMapAt, shadowMapUp);

		// 2022.5.15 目前平行光 proj 的矩阵方案，对z的范围取值很保守。可以改进
		Matrix mxShadowProj = XMMatrixOrthographicOffCenterLH(-sphereRadius, sphereRadius, -sphereRadius, sphereRadius, 0.0f, backDistance * 2.0f);

		// 更新当前 cascade 层 的 ShadowMap view proj 绘制矩阵
		m_cbDataCSMViewProj[i].view = mxShadowView.Transpose();
		m_cbDataCSMViewProj[i].projection = mxShadowProj.Transpose();
		m_cbCSMViewProj[i].Update(m_cbDataCSMViewProj[i]);
		g_cbDataShadowTest.view[i] = mxShadowView.Transpose();
		g_cbDataShadowTest.projection[i] = mxShadowProj.Transpose();

		auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["ShadowMap"]);
		auto pCSMDepthDSV = m_pTexCSMDepth->GetDSV(i);
		pCmdList->ClearDepthStencilView(pCSMDepthDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
		pCmdList->OMSetRenderTargets(0, nullptr, false, &pCSMDepthDSV);
		pCmdList->SetGraphicsRootConstantBufferView(1, m_cbCSMViewProj[i].CurrentGPUAddress());
		
		// 更新当前 cascade 层 的 ShadowMap world 绘制矩阵，并绘制
		for (auto pRenderableObj : m_scene->GetRenderableObjects())
		{
			RenderSingleObject(pCmdList, pRenderableObj);
		}
	}

	// Shadow Test
	g_cbShadowTest.Update(g_cbDataShadowTest);
}

void Renderer::RenderSingleObject(ID3D12GraphicsCommandList* pCmdList, NXRenderableObject* pRenderableObject)
{
	NXPrimitive* pPrimitive = pRenderableObject->IsPrimitive();
	if (pPrimitive)
	{
		pPrimitive->Update(pCmdList);

		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubmesh = pPrimitive->GetSubMesh(i);
			pSubmesh->Render(pCmdList);
		}
	}

	for (auto pChildObject : pRenderableObject->GetChilds())
	{
		NXRenderableObject* pChildRenderableObject = pChildObject->IsRenderableObject();
		if (pChildRenderableObject)
			RenderSingleObject(pCmdList, pChildRenderableObject);
	}
}
