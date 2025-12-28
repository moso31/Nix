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
#include "NXPassMaterialManager.h"
#include "NXPBRLight.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"
#include "NXAllocatorManager.h"

Renderer::Renderer(const Vector2& rtSize) :
	m_bRenderGUI(true),
	m_pRenderGraph(nullptr),
	m_viewRTSize(rtSize),
	m_bEnablePostProcessing(true),
	m_bEnableDebugLayer(false),
	m_bEnableShadowMapDebugLayer(false),
	m_fShadowMapZoomScale(1.0f)
{
	m_cbDebugLayerData = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
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

	NXPassMng->Init();

	InitGUI();
}

void Renderer::OnResize(const Vector2& rtSize)
{
	m_viewRTSize = rtSize;

	m_pFinalRT = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture("Final RT", DXGI_FORMAT_R11G11B10_FLOAT, (uint32_t)m_viewRTSize.x, (uint32_t)m_viewRTSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	m_scene->OnResize(rtSize);
}

void Renderer::InitGUI()
{
	m_pGUI = new NXGUI(m_scene, this);
	m_pGUI->Init();
	m_pGUI->SetVTReadbackData(m_vtReadbackData);
}

void Renderer::GenerateRenderGraph()
{
	// 这里的 RenderGraph 设计还比较初级，后续可能会有较大改动。目前规则：
	// setup：
	// - 准确的Read Write所需资源。Consume视为Read，Append视为Write（如果RW行为都有，就都调用）
	// - indirect args资源提交时，视为Read; 
	// execute:
	// - pCmdList目前直接显式暴露，包括资源状态切换、设置indirectArgs、乃至一些memcpy行为、全手动处理

	struct GPUTerrainPatcherData
	{
		NXRGHandle pMinMaxZMap;
		NXRGHandle pFinal;
		NXRGHandle pIndiArgs;
		NXRGHandle pPatcher;
		NXRGHandle pDrawIndexArgs;
	};
	NXRGPassNode<GPUTerrainPatcherData>* passPatcher = nullptr;

	auto terrIns = NXGPUTerrainManager::GetInstance();
	if (g_debug_temporal_enable_terrain_debug)
	{
		auto& pStreamingData = m_pTerrainLODStreamer->GetStreamingData();

		// 仅首帧执行
		// 清空Sector2NodeID纹理，将所有像素设置为65535 (0xFFFF)
		if (pStreamingData.NeedClearSector2NodeIDTexture())
		{
			struct TerrainSector2NodeClear
			{
				NXRGHandle Sector2NodeTex;
			};
			m_pRenderGraph->AddPass<TerrainSector2NodeClear>("Terrain Sector2Node Clear",
				[&](NXRGBuilder& builder, TerrainSector2NodeClear& data)
				{
					data.Sector2NodeTex = builder.Write(m_pRenderGraph->Import(pStreamingData.GetSector2NodeIDTexture()));
				},
				[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainSector2NodeClear& data)
				{
					auto pTex = resMap.GetRes(data.Sector2NodeTex).As<NXTexture2D>();
					pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

					// 使用 ClearUnorderedAccessViewUint 清空所有mip级别
					UINT clearValues[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
					int mipLevels = pTex->GetMipLevels();
					for (int i = 0; i < mipLevels; i++)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pTex->GetUAV(i);
						NXShVisDescHeap->PushFluid(cpuHandle);
						D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
						pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pTex->GetD3DResource(), clearValues, 0, nullptr);
					}

					pStreamingData.MarkSector2NodeIDTextureCleared();
				});
		}

		int groupNum = pStreamingData.GetNodeDescUpdateIndicesNum();
		if (groupNum > 0)
		{
			struct TerrainNodeDescCopy
			{
				NXRGHandle pNodeDescArrayGPU;
			};
			NXRGHandle pTerrNodeDescArrayGPU = m_pRenderGraph->Import(pStreamingData.GetNodeDescArrayGPUBuffer());

			m_pRenderGraph->AddPass<TerrainNodeDescCopy>("Terrain NodeDesc Copy",
				[&](NXRGBuilder& builder, TerrainNodeDescCopy& data) {
					data.pNodeDescArrayGPU = builder.Write(pTerrNodeDescArrayGPU);
				},
				[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainNodeDescCopy& data) {
					uint64_t pBufferOffset;
					auto& cbNodeDescArray = pStreamingData.GetNodeDescArray();
					auto pBuffer = cbNodeDescArray.CurrentGPUResourceAndOffset(pBufferOffset);
					auto pGPUBuffer = resMap.GetRes(data.pNodeDescArrayGPU).As<NXBuffer>();
					pCmdList->CopyBufferRegion(pGPUBuffer->GetD3DResource(), 0, pBuffer, pBufferOffset, cbNodeDescArray.GetByteSize());
				});

			struct TerrainAtlasBaker
			{
				std::vector<NXRGHandle> pIn;
				NXRGHandle pOutAtlas;
			};

			m_pRenderGraph->AddPass<TerrainAtlasBaker>("Terrain Atlas Baker: Height",
				[&, groupNum](NXRGBuilder& builder, TerrainAtlasBaker& data) {
					data.pIn.resize(groupNum);
					for (int i = 0; i < groupNum; i++)
					{
						data.pIn[i] = builder.Read(m_pRenderGraph->Import(pStreamingData.GetToAtlasHeightTextures()[i]));
					}
					data.pOutAtlas = builder.Write(m_pRenderGraph->Import(pStreamingData.GetHeightMapAtlas()));
				},
				[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainAtlasBaker& data) {
					auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainAtlasBaker"));
					for (int i = 0; i < groupNum; i++)
						pMat->SetInput(0, i, resMap.GetRes(data.pIn[i]));
					pMat->SetOutput(0, 0, resMap.GetRes(data.pOutAtlas));
					pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescUpdateIndices());

					uint32_t threadGroups = (pStreamingData.s_atlasSplatMapSize + 7) / 8;
					pMat->RenderSetTargetAndState(pCmdList);
					pMat->RenderBefore(pCmdList);
					pCmdList->Dispatch(threadGroups, threadGroups, groupNum);
				});

			m_pRenderGraph->AddPass<TerrainAtlasBaker>("Terrain Atlas Baker: Splat",
				[&, groupNum](NXRGBuilder& builder, TerrainAtlasBaker& data) {
					data.pIn.resize(groupNum);
					for (int i = 0; i < groupNum; i++)
					{
						data.pIn[i] = builder.Read(m_pRenderGraph->Import(pStreamingData.GetToAtlasSplatTextures()[i]));
					}
					data.pOutAtlas = builder.Write(m_pRenderGraph->Import(pStreamingData.GetSplatMapAtlas()));
				},
				[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainAtlasBaker& data) {
					auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainAtlasBaker"));
					for (int i = 0; i < groupNum; i++)
					{
						pMat->SetInput(0, i, resMap.GetRes(data.pIn[i]));
					}
					pMat->SetOutput(0, 0, resMap.GetRes(data.pOutAtlas));
					pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescUpdateIndices());

					uint32_t threadGroups = (pStreamingData.s_atlasSplatMapSize + 7) / 8;
					pMat->RenderSetTargetAndState(pCmdList);
					pMat->RenderBefore(pCmdList);
					pCmdList->Dispatch(threadGroups, threadGroups, groupNum);
				});

			struct TerrainSector2NodeTint
			{
				NXRGHandle Sector2NodeTex;
			};
			m_pRenderGraph->AddPass<TerrainSector2NodeTint>("Terrain Sector2Node Tint", 
				[&](NXRGBuilder& builder, TerrainSector2NodeTint& data) 
				{
					data.Sector2NodeTex = builder.Write(m_pRenderGraph->Import(pStreamingData.GetSector2NodeIDTexture()));
				},
				[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainSector2NodeTint& data) 
				{
					auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainSector2NodeTint"));
					pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescArray());
					pMat->SetConstantBuffer(0, 1, &pStreamingData.GetNodeDescUpdateIndices());
					for (int i = 0; i < 6; i++)
					{
						pMat->SetOutput(0, i, resMap.GetRes(data.Sector2NodeTex), i);
					}

					pMat->RenderSetTargetAndState(pCmdList);
					pMat->RenderBefore(pCmdList);
					pCmdList->Dispatch(groupNum, 1, 1);
				}
			);
		}

		NXRGHandle pTerrainBufferA = m_pRenderGraph->Import(terrIns->GetTerrainBufferA());
		NXRGHandle pTerrainBufferB = m_pRenderGraph->Import(terrIns->GetTerrainBufferB());
		NXRGHandle pTerrainBufferFinal = m_pRenderGraph->Import(terrIns->GetTerrainFinalBuffer());
		NXRGHandle pTerrainIndiArgs = m_pRenderGraph->Import(terrIns->GetTerrainIndirectArgs());
		NXRGHandle pTerrainPatcher = m_pRenderGraph->Import(terrIns->GetTerrainPatcherBuffer());
		NXRGHandle pTerrainDrawIndexArgs = m_pRenderGraph->Import(terrIns->GetTerrainDrawIndexArgs());

		struct FillTestData
		{
			NXRGHandle pIn;
			NXRGHandle pOut;
			NXRGHandle pFinal;
			NXRGHandle pIndiArgs;
		};

		std::vector<NXRGPassNode<FillTestData>*> terrainFillPasses(6);
		auto& passFillResult = terrainFillPasses.back();
		for (int i = 0; i < 6; i++)
		{
			auto passPrevFill = (i == 0) ? nullptr : terrainFillPasses[i - 1];
			auto& passFill = terrainFillPasses[i];

			std::string strBufName = "Terrain Fill " + std::to_string(i);
			passFill = m_pRenderGraph->AddPass<FillTestData>(strBufName,
				[=](NXRGBuilder& builder, FillTestData& data) {
					data.pIndiArgs = builder.Read(pTerrainIndiArgs);
					data.pFinal = builder.Write(pTerrainBufferFinal);

					if (i == 0)
					{
						data.pIn = builder.Read(pTerrainBufferA);
						data.pOut = builder.Write(pTerrainBufferB);
					}
					else
					{
						data.pIn = builder.Read(passPrevFill->GetData().pOut);
						data.pOut = builder.Write(passPrevFill->GetData().pIn);
					}
				},
				[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, FillTestData& data) mutable {
					auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainFillTest"));
					pMat->SetOutput(0, 0, resMap.GetRes(data.pIn));
					pMat->SetOutput(0, 1, resMap.GetRes(data.pOut));
					pMat->SetOutput(0, 2, resMap.GetRes(data.pFinal));
					pMat->SetConstantBuffer(0, 0, &NXGPUTerrainManager::GetInstance()->GetCBTerrainParams(i));

					Ntr<NXBuffer> pInputRes = resMap.GetRes(data.pIn).As<NXBuffer>();
					Ntr<NXBuffer> pOutputRes = resMap.GetRes(data.pOut).As<NXBuffer>();
					Ntr<NXBuffer> pIndiArgsRes = resMap.GetRes(data.pIndiArgs).As<NXBuffer>();
					Ntr<NXBuffer> pFinalRes = resMap.GetRes(data.pFinal).As<NXBuffer>();
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

						pInputRes->SetCurrent(initData.data(), initData.size());
						pFinalRes->SetCurrent(nullptr, 0);

						pInputRes->WaitForUploadFinish();
						pFinalRes->WaitForUploadFinish();
					}
					NXGPUTerrainManager::GetInstance()->UpdateLodParams(i);

					// 拷贝pInput.UAV计数器 作为 dispatch indirect args
					// 虽然只是拷贝pInput.UAV计数器，但目前的设计不太灵活，要SetResourceState必须带着原始资源一起做...
					pInputRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
					pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
					// ...不过最终拷贝的时候，只拷贝pInput.UAV计数器即可。
					pCmdList->CopyBufferRegion(pIndiArgsRes->GetD3DResource(), 0, pInputRes->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

					pMat->RenderSetTargetAndState(pCmdList);
					pMat->RenderBefore(pCmdList);

					pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pIndiArgsRes->GetD3DResource(), 0, nullptr, 0);
				});
		}

		auto passPatchClear = m_pRenderGraph->AddPass<GPUTerrainPatcherData>("GPU Terrain Patcher Clear",
			[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
				data.pPatcher = builder.Write(pTerrainPatcher);
				data.pDrawIndexArgs = builder.Write(pTerrainDrawIndexArgs);
			},
			[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GPUTerrainPatcherData& data) {
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainGPUPatcher:clear"));
				pMat->SetOutput(0, 0, resMap.GetRes(data.pPatcher));
				pMat->SetOutput(0, 1, resMap.GetRes(data.pDrawIndexArgs));
				pMat->SetOutput(0, 2, resMap.GetRes(data.pPatcher), true);

				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);

				pCmdList->Dispatch(1, 1, 1);
			});

		auto pTerrain_MinMaxZMap2DArray = m_pRenderGraph->Import(terrIns->GetTerrainMinMaxZMap2DArray());
		passPatcher = m_pRenderGraph->AddPass<GPUTerrainPatcherData>("GPU Terrain Patcher",
			[=](NXRGBuilder& builder, GPUTerrainPatcherData& data) {
				data.pMinMaxZMap = builder.Read(pTerrain_MinMaxZMap2DArray);
				data.pFinal = builder.Read(passFillResult->GetData().pFinal);
				data.pIndiArgs = builder.Read(passFillResult->GetData().pIndiArgs);
				data.pPatcher = builder.Write(passPatchClear->GetData().pPatcher);
				data.pDrawIndexArgs = builder.Write(passPatchClear->GetData().pDrawIndexArgs);
			},
			[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GPUTerrainPatcherData& data) mutable {
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainGPUPatcher:patch"));
				pMat->SetConstantBuffer(0, 1, &g_cbCamera);
				pMat->SetConstantBuffer(0, 2, &NXGPUTerrainManager::GetInstance()->GetTerrainSupportParam());
				pMat->SetInput(0, 0, resMap.GetRes(data.pMinMaxZMap));
				pMat->SetInput(0, 1, resMap.GetRes(data.pFinal));
				pMat->SetOutput(0, 0, resMap.GetRes(data.pPatcher));
				pMat->SetOutput(0, 1, resMap.GetRes(data.pDrawIndexArgs));

				Ntr<NXBuffer> pIndiArgsRes = resMap.GetRes(data.pIndiArgs).As<NXBuffer>();
				Ntr<NXBuffer> pFinalRes = resMap.GetRes(data.pFinal).As<NXBuffer>();

				// 虽然只是拷贝pInput.UAV计数器，但目前的设计不太灵活，要SetResourceState必须带着原始资源一起做...
				pFinalRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
				pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
				// ...不过最终拷贝的时候，只拷贝pInput.UAV计数器即可。
				pCmdList->CopyBufferRegion(pIndiArgsRes->GetD3DResource(), 0, pFinalRes->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);

				pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pIndiArgsRes->GetD3DResource(), 0, nullptr, 0);
			});
	}

	NXRGHandle hGBuffer0 = m_pRenderGraph->Create("GBuffer RT0", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer1 = m_pRenderGraph->Create("GBuffer RT1", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer2 = m_pRenderGraph->Create("GBuffer RT2", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R10G10B10A2_UNORM, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hGBuffer3 = m_pRenderGraph->Create("GBuffer RT3", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R8G8B8A8_UNORM, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle hDepthZ = m_pRenderGraph->Create("DepthZ", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::DepthStencil, .tex = { .format = DXGI_FORMAT_R24G8_TYPELESS, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

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
			if (passPatcher)
				builder.Read(passPatcher->GetData().pPatcher); 

			data.rt0	= builder.Write(hGBuffer0);
			data.rt1	= builder.Write(hGBuffer1);
			data.rt2	= builder.Write(hGBuffer2);
			data.rt3	= builder.Write(hGBuffer3);
			data.depth	= builder.Write(hDepthZ);
		}, 
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GBufferData& data) {
			// GBuffer 和其他的pass不太一样，依赖动态变化的场景/材质信息
			Ntr<NXTexture> pOutRTs[] = {
				resMap.GetRes(data.rt0),
				resMap.GetRes(data.rt1),
				resMap.GetRes(data.rt2),
				resMap.GetRes(data.rt3)
			};
			Ntr<NXTexture> pOutDS = resMap.GetRes(data.depth);

			auto* pPassMaterial = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("GBuffer"));
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
							if (pSubMesh->IsSubMeshTerrain() && g_debug_temporal_enable_terrain_debug)
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

	NXRGHandle pVTReadback = m_pRenderGraph->Create("VT Readback Buffer", { .resourceType = NXResourceType::Buffer, .usage = NXRGResourceUsage::UnorderedAccess, .buf = { .stride = 4, .arraySize = (uint32_t)(m_viewRTSize.x * m_viewRTSize.y * 0.125f * 0.125f) } });
	struct VTReadback
	{
		NXRGHandle gBuffer0;
		NXRGHandle vtReadback;
	};
	auto vtReadbackPassData = m_pRenderGraph->AddPass<VTReadback>("VTReadbackPass",
		[&](NXRGBuilder& builder, VTReadback& data) {
			data.gBuffer0 = builder.Read(gBufferPassData->GetData().rt0);
			data.vtReadback = builder.Write(pVTReadback);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, VTReadback& data) {
			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("VTReadback"));
			pMat->SetConstantBuffer(0, 0, &NXVirtualTextureManager::GetInstance()->GetCBufferVTReadback());
			pMat->SetInput(0, 0, resMap.GetRes(data.gBuffer0));
			pMat->SetOutput(0, 0, resMap.GetRes(data.vtReadback));

			auto& pRT = resMap.GetRes(data.gBuffer0);
			Int2 rtSize(pRT->GetWidth(), pRT->GetHeight());
			Int2 threadGroupSize((rtSize + 7) / 8);

			// 记录VTReadback的size 
			m_vtReadbackDataSize = threadGroupSize;
			m_pGUI->SetVTReadbackDataSize(threadGroupSize);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(threadGroupSize.x, threadGroupSize.y, 1);
		});

	struct VTReadbackData
	{
		NXRGHandle vtReadback;
	};
	auto vtReadbackDataPassData = m_pRenderGraph->AddPass<VTReadbackData>("DoVTReadback",
		[&](NXRGBuilder& builder, VTReadbackData& data) {
			data.vtReadback = builder.Read(vtReadbackPassData->GetData().vtReadback);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, VTReadbackData& data) {
			auto pMat = static_cast<NXReadbackPassMaterial*>(NXPassMng->GetPassMaterial("VTReadbackData"));
			pMat->SetInput(resMap.GetRes(data.vtReadback));
			pMat->SetOutput(m_vtReadbackData);
			pMat->Render(pCmdList);
		});

	struct ShadowMapData
	{
		NXRGHandle csmDepth;
	};

	NXRGHandle pCSMDepth = m_pRenderGraph->Import(m_pTexCSMDepth);
	auto shadowMapPassData = m_pRenderGraph->AddPass<ShadowMapData>("ShadowMap",
		[&](NXRGBuilder& builder, ShadowMapData& data) {
			if (passPatcher)
				builder.Read(passPatcher->GetData().pPatcher);

			data.csmDepth = builder.Write(pCSMDepth);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, ShadowMapData& data) {
			auto vpCamera = NX12Util::ViewPort((float)m_shadowMapRTSize, (float)m_shadowMapRTSize);
			pCmdList->RSSetViewports(1, &vpCamera);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));

			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("ShadowMap"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
			pMat->SetOutputDS(resMap.GetRes(data.csmDepth));

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
		NXRGHandle gbufferDepth;
		NXRGHandle csmDepth;
		NXRGHandle shadowTest;
	};

	NXRGHandle pShadowTest = m_pRenderGraph->Create("ShadowTest RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	auto shadowTestPassData = m_pRenderGraph->AddPass<ShadowTestData>("ShadowTest",
		[&](NXRGBuilder& builder, ShadowTestData& data) {
			data.gbufferDepth = builder.Read(gBufferPassData->GetData().depth);
			data.csmDepth = builder.Read(shadowMapPassData->GetData().csmDepth);
			data.shadowTest = builder.Write(pShadowTest);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, ShadowTestData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("ShadowTest"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
			pMat->SetInputTex(0, 0, resMap.GetRes(data.gbufferDepth));
			pMat->SetInputTex(0, 1, resMap.GetRes(data.csmDepth));
			pMat->SetOutputRT(0, resMap.GetRes(data.shadowTest));

			auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
			pCmdList->RSSetViewports(1, &vpCamera);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
			pMat->Render(pCmdList);
		});

	struct DeferredLightingData
	{
		NXRGHandle gbuffer0;
		NXRGHandle gbuffer1;
		NXRGHandle gbuffer2;
		NXRGHandle gbuffer3;
		NXRGHandle gbufferDepth;
		NXRGHandle shadowTest;
		NXRGHandle cubeMap;
		NXRGHandle preFilter;
		NXRGHandle brdfLut;
		NXRGHandle lighting;
		NXRGHandle lightingSpec;
		NXRGHandle lightingCopy;
	};

	NXRGHandle pLit = m_pRenderGraph->Create("Lighting RT0", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pLitSpec = m_pRenderGraph->Create("Lighting RT1", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pLitCopy = m_pRenderGraph->Create("Lighting RT Copy", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pCubeMap		= m_pRenderGraph->Import(m_scene->GetCubeMap()->GetCubeMap());
	NXRGHandle pPreFilter	= m_pRenderGraph->Import(m_scene->GetCubeMap()->GetPreFilterMap());
	NXRGHandle pBRDFLut		= m_pRenderGraph->Import(m_pBRDFLut->GetTex());

	auto litPassData = m_pRenderGraph->AddPass<DeferredLightingData>("DeferredLighting",
		[&](NXRGBuilder& builder, DeferredLightingData& data) {
			data.gbuffer0 = builder.Read(gBufferPassData->GetData().rt0);
			data.gbuffer1 = builder.Read(gBufferPassData->GetData().rt1);
			data.gbuffer2 = builder.Read(gBufferPassData->GetData().rt2);
			data.gbuffer3 = builder.Read(gBufferPassData->GetData().rt3);
			data.gbufferDepth = builder.Read(gBufferPassData->GetData().depth);
			data.shadowTest = builder.Read(shadowTestPassData->GetData().shadowTest);
			data.cubeMap = builder.Read(pCubeMap);
			data.preFilter = builder.Read(pPreFilter);
			data.brdfLut = builder.Read(pBRDFLut);
			data.lighting = builder.Write(pLit); 
			data.lightingSpec = builder.Write(pLitSpec); 
			data.lightingCopy = builder.Write(pLitCopy); 
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, DeferredLightingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("DeferredLighting"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject); // b0
			pMat->SetConstantBuffer(0, 1, &g_cbCamera); // b1
			pMat->SetConstantBuffer(0, 2, &m_scene->GetConstantBufferLights()); // b2
			pMat->SetConstantBuffer(0, 3, &m_scene->GetCubeMap()->GetCBDataParams()); // b3
			pMat->SetConstantBuffer(0, 4, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile()); // b4

			pMat->SetInputTex(0, 0, resMap.GetRes(data.gbuffer0)); // register t0
			pMat->SetInputTex(0, 1, resMap.GetRes(data.gbuffer1)); // t1
			pMat->SetInputTex(0, 2, resMap.GetRes(data.gbuffer2)); // t2
			pMat->SetInputTex(0, 3, resMap.GetRes(data.gbuffer3)); // t3
			pMat->SetInputTex(0, 4, resMap.GetRes(data.gbufferDepth)); // t4
			pMat->SetInputTex(0, 5, resMap.GetRes(data.shadowTest)); // t5
			pMat->SetInputTex(0, 6, resMap.GetRes(data.cubeMap));
			pMat->SetInputTex(0, 7, resMap.GetRes(data.preFilter));
			pMat->SetInputTex(0, 8, resMap.GetRes(data.brdfLut));
			pMat->SetOutputRT(0, resMap.GetRes(data.lighting)); 
			pMat->SetOutputRT(1, resMap.GetRes(data.lightingSpec)); 
			pMat->SetOutputRT(2, resMap.GetRes(data.lightingCopy)); 

			pMat->Render(pCmdList);
		});

	struct SubsurfaceData
	{
		NXRGHandle lighting;
		NXRGHandle lightingSpec;
		NXRGHandle gbuffer1;
		NXRGHandle noise64;
		NXRGHandle buf;
		NXRGHandle depth;
	};
	NXRGHandle pNoise64 = m_pRenderGraph->Import(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64));
	auto sssPassData = m_pRenderGraph->AddPass<SubsurfaceData>("Subsurface",
		[&](NXRGBuilder& builder, SubsurfaceData& data) {
			data.lighting = builder.Read(litPassData->GetData().lighting);
			data.lightingSpec = builder.Read(litPassData->GetData().lightingSpec);
			data.gbuffer1 = builder.Read(gBufferPassData->GetData().rt1);
			data.noise64 = builder.Read(pNoise64);
			data.depth	= builder.ReadWrite(gBufferPassData->GetData().depth);
			data.buf	= builder.ReadWrite(litPassData->GetData().lightingCopy);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, SubsurfaceData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("Subsurface"));
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 3, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

			pMat->SetInputTex(0, 0, resMap.GetRes(data.lighting));
			pMat->SetInputTex(0, 1, resMap.GetRes(data.lightingSpec));
			pMat->SetInputTex(0, 3, resMap.GetRes(data.gbuffer1));
			pMat->SetInputTex(0, 4, resMap.GetRes(data.depth));
			pMat->SetInputTex(0, 5, resMap.GetRes(data.noise64));
			pMat->SetOutputRT(0, resMap.GetRes(data.buf));
			pMat->SetOutputDS(resMap.GetRes(data.depth));

			pMat->Render(pCmdList);
		});

	struct SkyLightingData
	{
		NXRGHandle cubeMap;
		NXRGHandle buf;
		NXRGHandle depth;
	};
	auto skyPassData = m_pRenderGraph->AddPass<SkyLightingData>("SkyLighting",
		[&](NXRGBuilder& builder, SkyLightingData& data) {
			data.cubeMap = builder.Read(pCubeMap);
			data.buf	= builder.ReadWrite(sssPassData->GetData().buf);
			data.depth	= builder.ReadWrite(gBufferPassData->GetData().depth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, SkyLightingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("SkyLighting"));
			pMat->SetConstantBuffer(0, 0, &m_scene->GetCubeMap()->GetCBObjectParams());
			pMat->SetConstantBuffer(0, 1, &m_scene->GetCubeMap()->GetCBDataParams());
			pMat->SetInputTex(0, 0, resMap.GetRes(data.cubeMap));
			pMat->SetOutputRT(0, resMap.GetRes(data.buf));
			pMat->SetOutputDS(resMap.GetRes(data.depth));

			pMat->Render(pCmdList);
		});

	struct PostProcessingData
	{
		NXRGHandle skyBuf;
		NXRGHandle out;
	};

	NXRGHandle pPostProcess = m_pRenderGraph->Create("PostProcessing RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	auto postProcessPassData = m_pRenderGraph->AddPass<PostProcessingData>("PostProcessing",
		[&](NXRGBuilder& builder, PostProcessingData& data) {
			data.skyBuf = builder.Read(skyPassData->GetData().buf);
			data.out = builder.Write(pPostProcess);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PostProcessingData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("PostProcessing"));
			
			m_cbColorMappingData.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
			m_cbColorMapping.Update(m_cbColorMappingData);
			
			pMat->SetConstantBuffer(0, 2, &m_cbColorMapping);
			pMat->SetInputTex(0, 0, resMap.GetRes(data.skyBuf));
			pMat->SetOutputRT(0, resMap.GetRes(data.out));

			pMat->Render(pCmdList);
		});

	struct DebugLayerData
	{
		NXRGHandle postProcessOut;
		NXRGHandle csmDepth;
		NXRGHandle out;
	};

	NXRGHandle pDebugLayer = m_pRenderGraph->Create("Debug Layer RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	auto debugLayerPassData = m_pRenderGraph->AddPass<DebugLayerData>("DebugLayer",
		[&](NXRGBuilder& builder, DebugLayerData& data) {
			data.postProcessOut = builder.Read(postProcessPassData->GetData().out);
			data.csmDepth = builder.Read(shadowMapPassData->GetData().csmDepth);
			data.out = builder.Write(pDebugLayer);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, DebugLayerData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("DebugLayer"));
			
			m_cbDebugLayerData.x = (float)m_bEnableShadowMapDebugLayer;
			m_cbDebugLayerData.y = m_fShadowMapZoomScale;
			m_cbDebugLayer.Update(m_cbDebugLayerData);
			
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &m_cbDebugLayer);
			pMat->SetInputTex(0, 0, resMap.GetRes(data.postProcessOut));
			pMat->SetInputTex(0, 1, resMap.GetRes(data.csmDepth));
			pMat->SetOutputRT(0, resMap.GetRes(data.out));

			pMat->Render(pCmdList);
		});

	struct GizmosData
	{
		NXRGHandle out;
	};
	auto gizmosPassData = m_pRenderGraph->AddPass<GizmosData>("Gizmos",
		[&](NXRGBuilder& builder, GizmosData& data) {
			NXRGHandle pOut = m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out;
			data.out = builder.ReadWrite(pOut);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GizmosData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("Gizmos"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, nullptr);
			pMat->SetOutputRT(0, resMap.GetRes(data.out));

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

	NXRGHandle pFinalRT = m_pRenderGraph->Import(m_pFinalRT);
	struct FinalQuadData
	{
		NXRGHandle gizmosOut;
		NXRGHandle finalOut;
	};
	auto FinalQuadPassData = m_pRenderGraph->AddPass<FinalQuadData>("FinalQuad",
		[&](NXRGBuilder& builder, FinalQuadData& data) {
			data.gizmosOut = builder.Read(gizmosPassData->GetData().out);
			data.finalOut = builder.Write(pFinalRT);
 		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, FinalQuadData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("FinalQuad"));
			pMat->SetInputTex(0, 0, resMap.GetRes(data.gizmosOut));
			pMat->SetOutputRT(0, resMap.GetRes(data.finalOut));

			pMat->Render(pCmdList);
		});
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
	m_pTexCSMDepth = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture2DArray("CSM DepZ 2DArray", DXGI_FORMAT_R32_TYPELESS, m_shadowMapRTSize, m_shadowMapRTSize, m_cascadeCount, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false);
	m_pTexCSMDepth->SetViews(1, 0, m_cascadeCount, 0);
	for (UINT i = 0; i < m_cascadeCount; i++) m_pTexCSMDepth->SetDSV(i, i, 1);	// DSV 单张切片（每次写cascade深度 只写一片）
	m_pTexCSMDepth->SetSRV(0, 0, m_cascadeCount); // SRV 读取整个纹理数组（ShadowTest时使用）
	m_pTexCSMDepth->SetSRVPreviewsManual(1);
}

void Renderer::ResourcesReloading(DirectResources* pDXRes)
{
	NXResourceManager::GetInstance()->OnReload();
	NXResourceReloader::GetInstance()->OnReload();
}

void Renderer::Update()
{
	m_pTerrainLODStreamer->ProcessCompletedStreamingTask();
	m_pTerrainLODStreamer->Update();
	m_pTerrainLODStreamer->UpdateAsyncLoader();

	// 每帧都Compile RenderGraph
	m_pRenderGraph->Clear();
	GenerateRenderGraph();
	m_pRenderGraph->Compile();

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
	if (m_bRenderGUI && m_pFinalRT.IsValid())
	{
		m_pGUI->Render(m_pFinalRT, swapChainBuffer);
	}
}

void Renderer::Release()
{
	m_pRenderGraph->Clear();
	SafeDelete(m_pRenderGraph);

	SafeReleaseCOM(m_pCommandSig);

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

		auto pCSMDepthDSV = m_pTexCSMDepth->GetDSV(i);
		pCmdList->ClearDepthStencilView(pCSMDepthDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x0, 0, nullptr);
		pCmdList->OMSetRenderTargets(0, nullptr, false, &pCSMDepthDSV);
		pCmdList->SetGraphicsRootConstantBufferView(2, m_cbCSMViewProj[i].CurrentGPUAddress());
		
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
