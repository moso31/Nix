// RendererRenderGraph.cpp
// 将 GenerateRenderGraph() 拆分为多个辅助函数，改善 IntelliSense 性能
// PassData 结构体统一定义在 RenderGraphPassData.h 中

#include "pch.h"
#include "Renderer.h"
#include "RenderGraphPassData.h"

// ===== 依赖的头文件 =====
#include "NXGlobalDefinitions.h"
#include "NXBRDFlut.h"
#include "NXTerrainLODStreamer.h"
#include "NXVirtualTexture.h"
#include "NXRenderGraph.h"
#include "NXScene.h"
#include "NXGlobalBuffers.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXBuffer.h"
#include "NXCubeMap.h"
#include "NXRGPassNode.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"
#include "NXTerrainStreamingBatcher.h"
#include "NXPassMaterial.h"
#include "NXPassMaterialManager.h"
#include "NXPBRLight.h"
#include "NXPrimitive.h"
#include "NXEditorObjectManager.h"
#include "NXCamera.h"

// =====================================================
// Terrain Streaming Passes
// =====================================================

void Renderer::BuildTerrainStreamingPasses(NXRGHandle& hSector2IndirectTexture, NXRGHandle& pSector2NodeIDTex, NXRGHandle& hHeightMapAtlas, NXRGHandle& hSplatMapAtlas, NXRGHandle& hNormalMapAtlas)
{
	hSector2IndirectTexture = m_pRenderGraph->Import(m_pVirtualTexture->GetSector2IndirectTexture());

	// 仅首帧执行: 清空Sector2IndirectTexture，将所有像素设置为-1
	if (m_pVirtualTexture->NeedClearSector2IndirectTexture())
	{
		m_pRenderGraph->AddPass<Sector2IndirectTextureClearPassData>("Sector2IndirectTexture Clear",
			[&](NXRGBuilder& builder, Sector2IndirectTextureClearPassData& data)
			{
				data.Sector2IndirectTex = builder.Write(hSector2IndirectTexture);
			},
			[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, Sector2IndirectTextureClearPassData& data)
			{
				auto pTex = resMap.GetRes(data.Sector2IndirectTex).As<NXTexture2D>();
				pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				UINT clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pTex->GetUAV(0);
				NXShVisDescHeap->PushFluid(cpuHandle);
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
				pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pTex->GetD3DResource(), clearValues, 0, nullptr);

				m_pVirtualTexture->MarkSector2IndirectTextureCleared();
			});
	}

	m_pRenderGraph->AddPass<Sector2IndirectTexturePassData>("UpdateSector2IndirectTexture",
		[&](NXRGBuilder& builder, Sector2IndirectTexturePassData& data)
		{
			data.Sector2IndirectTexture = builder.Write(hSector2IndirectTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, Sector2IndirectTexturePassData& data)
		{
			uint32_t threadGroups = (m_pVirtualTexture->GetCBufferSector2IndirectTextureDataNum() + 7) / 8;
			if (threadGroups == 0)
				return;

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("UpdateSector2IndirectTexture"));
			pMat->SetOutput(0, 0, resMap.GetRes(data.Sector2IndirectTexture));
			pMat->SetConstantBuffer(0, 0, &m_pVirtualTexture->GetCBufferSector2IndirectTexture());
			pMat->SetConstantBuffer(0, 1, &m_pVirtualTexture->GetCBufferSector2IndirectTextureNum());

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
			pCmdList->Dispatch(threadGroups, 1, 1);
		});

	auto& pStreamingData = m_pTerrainLODStreamer->GetStreamingData();
	pSector2NodeIDTex = m_pRenderGraph->Import(pStreamingData.GetSector2NodeIDTexture());
	hHeightMapAtlas = m_pRenderGraph->Import(pStreamingData.GetHeightMapAtlas());
	hSplatMapAtlas = m_pRenderGraph->Import(pStreamingData.GetSplatMapAtlas());
	hNormalMapAtlas = m_pRenderGraph->Import(pStreamingData.GetNormalMapAtlas());

	// 仅首帧执行: 清空Sector2NodeID纹理，将所有像素设置为65535 (0xFFFF)
	if (pStreamingData.NeedClearSector2NodeIDTexture())
	{
		m_pRenderGraph->AddPass<TerrainSector2NodeClearPassData>("Terrain Sector2Node Clear",
			[&](NXRGBuilder& builder, TerrainSector2NodeClearPassData& data)
			{
				data.Sector2NodeTex = builder.Write(pSector2NodeIDTex);
			},
			[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainSector2NodeClearPassData& data)
			{
				auto pTex = resMap.GetRes(data.Sector2NodeTex).As<NXTexture2D>();
				pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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
		std::vector<NXRGHandle> hToAtlasHeightTextures(groupNum);
		std::vector<NXRGHandle> hToAtlasSplatTextures(groupNum);
		std::vector<NXRGHandle> hToAtlasNormalTextures(groupNum);
		for (int i = 0; i < groupNum; i++)
		{
			hToAtlasHeightTextures[i] = m_pRenderGraph->Import(pStreamingData.GetToAtlasHeightTextures()[i]);
			hToAtlasSplatTextures[i] = m_pRenderGraph->Import(pStreamingData.GetToAtlasSplatTextures()[i]);
			hToAtlasNormalTextures[i] = m_pRenderGraph->Import(pStreamingData.GetToAtlasNormalTextures()[i]);
		}

		m_pRenderGraph->AddPass<TerrainAtlasBakerPassData>("Terrain Atlas Baker: Height",
			[&, groupNum](NXRGBuilder& builder, TerrainAtlasBakerPassData& data) {
				data.pIn.resize(groupNum);
				for (int i = 0; i < groupNum; i++)
				{
					data.pIn[i] = builder.Read(hToAtlasHeightTextures[i]);
				}
				data.pOutAtlas = builder.Write(hHeightMapAtlas);
			},
			[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainAtlasBakerPassData& data) {
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainAtlasBaker"));
				for (int i = 0; i < groupNum; i++)
					pMat->SetInput(0, i, resMap.GetRes(data.pIn[i]));
				pMat->SetOutput(0, 0, resMap.GetRes(data.pOutAtlas));
				pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescUpdateIndices());

				uint32_t threadGroups = (g_terrainStreamConfig.AtlasHeightMapSize + 7) / 8;
				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);
				pCmdList->Dispatch(threadGroups, threadGroups, groupNum);
			});

		m_pRenderGraph->AddPass<TerrainAtlasBakerPassData>("Terrain Atlas Baker: Splat",
			[&, groupNum](NXRGBuilder& builder, TerrainAtlasBakerPassData& data) {
				data.pIn.resize(groupNum);
				for (int i = 0; i < groupNum; i++)
				{
					data.pIn[i] = builder.Read(hToAtlasSplatTextures[i]);
				}
				data.pOutAtlas = builder.Write(hSplatMapAtlas);
			},
			[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainAtlasBakerPassData& data) {
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainAtlasBaker"));
				for (int i = 0; i < groupNum; i++)
				{
					pMat->SetInput(0, i, resMap.GetRes(data.pIn[i]));
				}
				pMat->SetOutput(0, 0, resMap.GetRes(data.pOutAtlas));
				pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescUpdateIndices());

				uint32_t threadGroups = (g_terrainStreamConfig.AtlasHeightMapSize + 7) / 8;
				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);
				pCmdList->Dispatch(threadGroups, threadGroups, groupNum);
			});

		m_pRenderGraph->AddPass<TerrainAtlasBakerPassData>("Terrain Atlas Baker: Normal",
			[&, groupNum](NXRGBuilder& builder, TerrainAtlasBakerPassData& data) {
				data.pIn.resize(groupNum);
				for (int i = 0; i < groupNum; i++)
				{
					data.pIn[i] = builder.Read(hToAtlasNormalTextures[i]);
				}
				data.pOutAtlas = builder.Write(hNormalMapAtlas);
			},
			[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainAtlasBakerPassData& data) {
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainAtlasBaker:Float4"));
				for (int i = 0; i < groupNum; i++)
				{
					pMat->SetInput(0, i, resMap.GetRes(data.pIn[i]));
				}
				pMat->SetOutput(0, 1, resMap.GetRes(data.pOutAtlas)); // TerrainAtlasBaker:Float4 用u1 而不是u0
				pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescUpdateIndices());

				uint32_t threadGroups = (g_terrainStreamConfig.AtlasNormalMapSize + 7) / 8;
				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);
				pCmdList->Dispatch(threadGroups, threadGroups, groupNum);
			});

		m_pRenderGraph->AddPass<TerrainSector2NodeTintPassData>("Terrain Sector2Node Tint",
			[&](NXRGBuilder& builder, TerrainSector2NodeTintPassData& data)
			{
				data.Sector2NodeTex = builder.Write(pSector2NodeIDTex);
			},
			[&, groupNum](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainSector2NodeTintPassData& data)
			{
				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainSector2NodeTint"));
				pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescArray());
				pMat->SetConstantBuffer(0, 1, &pStreamingData.GetNodeDescUpdateIndices());
				for (int i = 0; i < g_terrainStreamConfig.LODSize; i++)
				{
					pMat->SetOutput(0, i, resMap.GetRes(data.Sector2NodeTex), i);
				}

				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);
				pCmdList->Dispatch(groupNum, 1, 1);
			}
		);
	}
}

// =====================================================
// Terrain Culling Passes
// =====================================================

NXRGPassNode<TerrainPatcherPassData>* Renderer::BuildTerrainCullingPasses(NXRGHandle pSector2NodeIDTex, NXRGHandle& hPatcherBuffer, NXRGHandle& hPatcherDrawIndexArgs)
{
	auto& pStreamingData = m_pTerrainLODStreamer->GetStreamingData();

	NXRGHandle pTerrainNodesA = m_pRenderGraph->Import(pStreamingData.GetPingPongNodesA());
	NXRGHandle pTerrainNodesB = m_pRenderGraph->Import(pStreamingData.GetPingPongNodesB());
	NXRGHandle pTerrainNodesFinal = m_pRenderGraph->Import(pStreamingData.GetPingPongNodesFinal());
	NXRGHandle pTerrainIndirectArgs = m_pRenderGraph->Import(pStreamingData.GetPingPongIndirectArgs());

	// 每帧clear一次FinalBuffer
	m_pRenderGraph->AddPass<TerrainNodesCullingPassData>("Terrain Nodes Clear",
		[=, &pStreamingData](NXRGBuilder& builder, TerrainNodesCullingPassData& data)
		{
			data.pFinal = builder.Write(pTerrainNodesFinal);
		},
		[=, &pStreamingData](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainNodesCullingPassData& data)
		{
			UINT clearValues[4] = { 0,0,0,0 };
			auto pGPUBuffer = resMap.GetRes(data.pFinal).As<NXBuffer>();
			pGPUBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pGPUBuffer->GetUAVCounter();
			NXShVisDescHeap->PushFluid(cpuHandle);
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
			pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pGPUBuffer->GetD3DResourceUAVCounter(), clearValues, 0, nullptr);
		});

	// culling：预加载 填满mip5初始nodeID
	m_pRenderGraph->AddPass<TerrainNodesCullingPassData>("Terrain Nodes Culling: First",
		[=, &pStreamingData](NXRGBuilder& builder, TerrainNodesCullingPassData& data)
		{
			data.pOut = builder.Write(pTerrainNodesA);
			data.pFinal = builder.Write(pTerrainNodesFinal);
			data.sector2NodeTex = builder.Read(pSector2NodeIDTex);
		},
		[=, &pStreamingData](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainNodesCullingPassData& data)
		{
			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainNodesCulling:First"));
			pMat->SetInput(0, 0, resMap.GetRes(data.sector2NodeTex)); // t0
			pMat->SetOutput(0, 1, resMap.GetRes(data.pOut)); // u1
			pMat->SetOutput(0, 2, resMap.GetRes(data.pFinal)); // u2
			pMat->SetConstantBuffer(0, 1, &pStreamingData.GetCullingParam(0));

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(1, 1, 1);
		});

	// culling：ping-pong
	NXRGPassNode<TerrainNodesCullingPassData>* pLastCullingPass = nullptr;
	for (int i = 0; i < g_terrainStreamConfig.LODSize; i++)
	{
		pLastCullingPass = m_pRenderGraph->AddPass<TerrainNodesCullingPassData>("Terrain Nodes Culling" + std::to_string(i),
			[=, &pStreamingData](NXRGBuilder& builder, TerrainNodesCullingPassData& data)
			{
				data.pIn = builder.Read((i % 2 == 0) ? pTerrainNodesA : pTerrainNodesB);
				data.pOut = builder.Write((i % 2 == 0) ? pTerrainNodesB : pTerrainNodesA);
				data.pFinal = builder.Write(pTerrainNodesFinal);
				data.pIndiArgs = builder.Read(pTerrainIndirectArgs);
				data.sector2NodeTex = builder.Read(pSector2NodeIDTex);
			},
			[=, &pStreamingData](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainNodesCullingPassData& data)
			{
				Ntr<NXBuffer> pInputRes = resMap.GetRes(data.pIn).As<NXBuffer>();
				Ntr<NXBuffer> pOutputRes = resMap.GetRes(data.pOut).As<NXBuffer>();
				Ntr<NXBuffer> pIndiArgsRes = resMap.GetRes(data.pIndiArgs).As<NXBuffer>();
				Ntr<NXBuffer> pFinalRes = resMap.GetRes(data.pFinal).As<NXBuffer>();

				auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainNodesCulling:Process"));
				pMat->SetInput(0, 0, resMap.GetRes(data.sector2NodeTex));
				pMat->SetOutput(0, 0, pInputRes);
				pMat->SetOutput(0, 1, pOutputRes);
				pMat->SetOutput(0, 2, pFinalRes);
				pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescArray());
				pMat->SetConstantBuffer(0, 1, &pStreamingData.GetCullingParam(i));

				pInputRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
				pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
				pCmdList->CopyBufferRegion(pIndiArgsRes->GetD3DResource(), 0, pInputRes->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

				pMat->RenderSetTargetAndState(pCmdList);
				pMat->RenderBefore(pCmdList);

				pIndiArgsRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pIndiArgsRes->GetD3DResource(), 0, nullptr, 0);
			});
	}

	hPatcherBuffer = m_pRenderGraph->Import(pStreamingData.GetPatcherBuffer());
	hPatcherDrawIndexArgs = m_pRenderGraph->Import(pStreamingData.GetPatcherDrawIndexArgs());

	m_pRenderGraph->AddPass<TerrainPatcherPassData>("Terrain Patcher Clear",
		[=, &pStreamingData](NXRGBuilder& builder, TerrainPatcherPassData& data) {
			data.pFinal = builder.Read(pTerrainNodesFinal);
			data.pPatcher = builder.Write(hPatcherBuffer);
			data.pIndirectArgs = builder.Read(pTerrainIndirectArgs);
			data.pDrawIndexArgs = builder.Write(hPatcherDrawIndexArgs);
		},
		[=, &pStreamingData](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainPatcherPassData& data) {
			UINT clearValues[4] = { 0,0,0,0 };
			auto pGPUBuffer = resMap.GetRes(data.pPatcher).As<NXBuffer>();
			pGPUBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pGPUBuffer->GetUAVCounter();
			NXShVisDescHeap->PushFluid(cpuHandle);
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
			pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pGPUBuffer->GetD3DResourceUAVCounter(), clearValues, 0, nullptr);

			auto pDrawIndexArgs = resMap.GetRes(data.pDrawIndexArgs).As<NXBuffer>();
			auto pZeroBuffer = pStreamingData.GetPatcherDrawIndexArgsZero();
			pZeroBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pDrawIndexArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
			pCmdList->CopyBufferRegion(pDrawIndexArgs->GetD3DResource(), 0, pZeroBuffer->GetD3DResource(), 0, sizeof(uint32_t) * 5);
		});

	return m_pRenderGraph->AddPass<TerrainPatcherPassData>("Terrain Patcher",
		[=, &pStreamingData](NXRGBuilder& builder, TerrainPatcherPassData& data) {
			data.pFinal = builder.Read(pTerrainNodesFinal);
			data.pPatcher = builder.Write(hPatcherBuffer);
			data.pIndirectArgs = builder.Read(pTerrainIndirectArgs);
			data.pDrawIndexArgs = builder.Write(hPatcherDrawIndexArgs);
		},
		[=, &pStreamingData](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, TerrainPatcherPassData& data) {
			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("TerrainPatcher"));
			pMat->SetInput(0, 0, resMap.GetRes(data.pFinal));
			pMat->SetOutput(0, 0, resMap.GetRes(data.pPatcher));
			pMat->SetOutput(0, 1, resMap.GetRes(data.pDrawIndexArgs));
			pMat->SetConstantBuffer(0, 0, &pStreamingData.GetNodeDescArray());
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);

			Ntr<NXBuffer> pBufFinal = resMap.GetRes(data.pFinal).As<NXBuffer>();
			Ntr<NXBuffer> pBufIndirectArgs = resMap.GetRes(data.pIndirectArgs).As<NXBuffer>();

			pBufFinal->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pBufIndirectArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
			pCmdList->CopyBufferRegion(pBufIndirectArgs->GetD3DResource(), 0, pBufFinal->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pBufIndirectArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pBufIndirectArgs->GetD3DResource(), 0, nullptr, 0);
		});
}

// =====================================================
// Virtual Texture Passes
// =====================================================

void Renderer::BuildVirtualTexturePasses(NXRGHandle pSector2NodeIDTex, NXRGHandle hSplatMapAtlas)
{
	auto& pStreamingData = m_pTerrainLODStreamer->GetStreamingData();

	NXRGHandle hAlbedoMapArray = m_pRenderGraph->Import(pStreamingData.GetTerrainAlbedo2DArray());
	NXRGHandle hNormalMapArray = m_pRenderGraph->Import(pStreamingData.GetTerrainNormal2DArray());
	NXRGHandle hAlbedoPhysicalPage = m_pRenderGraph->Import(m_pVirtualTexture->GetPhysicalPageAlbedo());
	NXRGHandle hNormalPhysicalPage = m_pRenderGraph->Import(m_pVirtualTexture->GetPhysicalPageNormal());

	m_pRenderGraph->AddPass<PhysicalPageBakerPassData>("PhysicalPageBaker",
		[&](NXRGBuilder& builder, PhysicalPageBakerPassData& data)
		{
			data.Sector2NodeIDTex = builder.Read(pSector2NodeIDTex);
			data.SplatMapAtlas = builder.Read(hSplatMapAtlas);
			data.AlbedoMapArray = builder.Read(hAlbedoMapArray);
			data.NormalMapArray = builder.Read(hNormalMapArray);
			data.PhysicalPageAlbedo = builder.Read(hAlbedoPhysicalPage);
			data.PhysicalPageNormal = builder.Read(hNormalPhysicalPage);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PhysicalPageBakerPassData& data)
		{
			uint32_t threadNum = (g_virtualTextureConfig.PhysicalPageTileNum + 7) / 8;
			uint32_t bakeTexNum = m_pVirtualTexture->GetCBPhysPageBakeDataNum();

			if (bakeTexNum == 0)
				return;

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("PhysicalPageBaker"));
			pMat->SetConstantBuffer(0, 0, &m_pVirtualTexture->GetCBPhysPageBakeData());
			pMat->SetConstantBuffer(0, 1, &pStreamingData.GetNodeDescArray());
			pMat->SetConstantBuffer(0, 2, &m_pVirtualTexture->GetCBPhysPageUpdateIndex());
			pMat->SetInput(0, 0, resMap.GetRes(data.Sector2NodeIDTex));
			pMat->SetInput(0, 1, resMap.GetRes(data.SplatMapAtlas));
			pMat->SetInput(0, 2, resMap.GetRes(data.AlbedoMapArray));
			pMat->SetInput(0, 3, resMap.GetRes(data.NormalMapArray));
			pMat->SetOutput(0, 0, resMap.GetRes(data.PhysicalPageAlbedo));
			pMat->SetOutput(0, 1, resMap.GetRes(data.PhysicalPageNormal));

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(threadNum, threadNum, bakeTexNum);
		});

	//NXRGHandle hIndirectTexture = m_pRenderGraph->Import(m_pVirtualTexture->GetIndirectTexture());
	//m_pRenderGraph->AddPass<UpdateIndirectTexturePassData>("UpdateIndirectTexture",
	//	[&](NXRGBuilder& builder, UpdateIndirectTexturePassData& data)
	//	{
	//		data.IndirectTexture = hIndirectTexture;
	//	},
	//	[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, UpdateIndirectTexturePassData& data)
	//	{
	//		auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("UpdateIndirectTexture"));
	//		//pMat->SetConstantBuffer(0, 0, m_pVirtualTexture->);
	//	});
}

// =====================================================
// GBuffer Passes
// =====================================================

NXRGPassNode<GBufferPassData>* Renderer::BuildGBufferPasses(NXRGPassNode<TerrainPatcherPassData>* passPatcher, NXRGHandle hGBuffer0, NXRGHandle hGBuffer1, NXRGHandle hGBuffer2, NXRGHandle hGBuffer3, NXRGHandle hDepthZ, NXRGHandle hVTPageIDTexture, NXRGHandle hVTSector2IndirectTexture)
{
	// 调试：清空VTPageIDTexture
	m_pRenderGraph->AddPass<PageIDTextureClearPassData>("VTPageIDTexture Clear",
		[&](NXRGBuilder& builder, PageIDTextureClearPassData& data)
		{
			data.VTPageIDTexture = builder.Write(hVTPageIDTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PageIDTextureClearPassData& data)
		{
			auto pTex = resMap.GetRes(data.VTPageIDTexture).As<NXTexture2D>();
			pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			UINT clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pTex->GetUAV(0);
			NXShVisDescHeap->PushFluid(cpuHandle);
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
			pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pTex->GetD3DResource(), clearValues, 0, nullptr);
		});

	return m_pRenderGraph->AddPass<GBufferPassData>("GBufferPass",
		[&, passPatcher](NXRGBuilder& builder, GBufferPassData& data) {
			if (passPatcher)
				builder.Read(passPatcher->GetData().pPatcher);
			data.VTSector2IndirectTexture = builder.Read(hVTSector2IndirectTexture);

			data.rt0 = builder.Write(hGBuffer0);
			data.rt1 = builder.Write(hGBuffer1);
			data.rt2 = builder.Write(hGBuffer2);
			data.rt3 = builder.Write(hGBuffer3);
			data.depth = builder.Write(hDepthZ);
			data.VTPageIDTexture = builder.Write(hVTPageIDTexture);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GBufferPassData& data) {
			Ntr<NXTexture> pOutRTs[] = {
				resMap.GetRes(data.rt0),
				resMap.GetRes(data.rt1),
				resMap.GetRes(data.rt2),
				resMap.GetRes(data.rt3)
			};
			Ntr<NXTexture> pOutDS = resMap.GetRes(data.depth);

			auto* pPassMaterial = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("GBuffer"));
			pPassMaterial->SetInput(2, 0, resMap.GetRes(data.VTSector2IndirectTexture));
			pPassMaterial->SetOutputUAV(0, 0, resMap.GetRes(data.VTPageIDTexture));

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
					pMat->Update();
				}

				if (pCustomMat && pCustomMat->GetCompileSuccess())
				{
					if (pCustomMat->GetShadingModel() == NXShadingModel::SubSurface)
					{
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
								{
									auto& pVTPageIDTexture = resMap.GetRes(data.VTPageIDTexture).As<NXTexture2D>();
									NXShVisDescHeap->PushFluid(pVTPageIDTexture->GetUAV(0));

									auto& pVTSector2IndirectTexture = resMap.GetRes(data.VTSector2IndirectTexture).As<NXTexture2D>();
									NXShVisDescHeap->PushFluid(pVTSector2IndirectTexture->GetSRV(0));

									auto& srvHandle = NXShVisDescHeap->Submit();
									pCmdList->SetGraphicsRootDescriptorTable(5, srvHandle);
								}

								m_pTerrainLODStreamer->GetStreamingData().UpdateGBufferPatcherData(pCmdList);
								((NXSubMeshTerrain*)pSubMesh)->Render(pCmdList, m_pTerrainLODStreamer->GetStreamingData().GetPatcherDrawIndexArgs());
								break;
							}

							pSubMesh->GetRenderableObject()->Update(pCmdList);
							pSubMesh->Render(pCmdList);
						}
					}
				}
			}
		});
}

// =====================================================
// Shadow Passes
// =====================================================

NXRGPassNode<ShadowMapPassData>* Renderer::BuildShadowMapPass(NXRGPassNode<TerrainPatcherPassData>* passPatcher)
{
	NXRGHandle pCSMDepth = m_pRenderGraph->Import(m_pTexCSMDepth);
	return m_pRenderGraph->AddPass<ShadowMapPassData>("ShadowMap",
		[&, passPatcher](NXRGBuilder& builder, ShadowMapPassData& data) {
			if (passPatcher)
				builder.Read(passPatcher->GetData().pPatcher);
			data.csmDepth = builder.Write(pCSMDepth);
		},
		[=](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, ShadowMapPassData& data) {
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
}

NXRGPassNode<ShadowTestPassData>* Renderer::BuildShadowTestPass(NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGPassNode<ShadowMapPassData>* shadowMapPassData)
{
	NXRGHandle pShadowTest = m_pRenderGraph->Create("ShadowTest RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	return m_pRenderGraph->AddPass<ShadowTestPassData>("ShadowTest",
		[&](NXRGBuilder& builder, ShadowTestPassData& data) {
			data.gbufferDepth = builder.Read(gBufferPassData->GetData().depth);
			data.csmDepth = builder.Read(shadowMapPassData->GetData().csmDepth);
			data.shadowTest = builder.Write(pShadowTest);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, ShadowTestPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("ShadowTest"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
			pMat->SetInput(0, 0, resMap.GetRes(data.gbufferDepth));
			pMat->SetInput(0, 1, resMap.GetRes(data.csmDepth));
			pMat->SetOutputRT(0, resMap.GetRes(data.shadowTest));

			auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
			pCmdList->RSSetViewports(1, &vpCamera);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
			pMat->Render(pCmdList);
		});
}

// =====================================================
// Lighting Passes
// =====================================================

NXRGPassNode<DeferredLightingPassData>* Renderer::BuildDeferredLightingPass(NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGPassNode<ShadowTestPassData>* shadowTestPassData)
{
	NXRGHandle pLit = m_pRenderGraph->Create("Lighting RT0", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pLitSpec = m_pRenderGraph->Create("Lighting RT1", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R32G32B32A32_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pLitCopy = m_pRenderGraph->Create("Lighting RT Copy", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });
	NXRGHandle pCubeMap = m_pRenderGraph->Import(m_scene->GetCubeMap()->GetCubeMap());
	NXRGHandle pPreFilter = m_pRenderGraph->Import(m_scene->GetCubeMap()->GetPreFilterMap());
	NXRGHandle pBRDFLut = m_pRenderGraph->Import(m_pBRDFLut->GetTex());

	return m_pRenderGraph->AddPass<DeferredLightingPassData>("DeferredLighting",
		[&](NXRGBuilder& builder, DeferredLightingPassData& data) {
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
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, DeferredLightingPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("DeferredLighting"));
			pMat->SetConstantBuffer(0, 0, &g_cbObject);
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &m_scene->GetConstantBufferLights());
			pMat->SetConstantBuffer(0, 3, &m_scene->GetCubeMap()->GetCBDataParams());
			pMat->SetConstantBuffer(0, 4, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

			pMat->SetInput(0, 0, resMap.GetRes(data.gbuffer0));
			pMat->SetInput(0, 1, resMap.GetRes(data.gbuffer1));
			pMat->SetInput(0, 2, resMap.GetRes(data.gbuffer2));
			pMat->SetInput(0, 3, resMap.GetRes(data.gbuffer3));
			pMat->SetInput(0, 4, resMap.GetRes(data.gbufferDepth));
			pMat->SetInput(0, 5, resMap.GetRes(data.shadowTest));
			pMat->SetInput(0, 6, resMap.GetRes(data.cubeMap));
			pMat->SetInput(0, 7, resMap.GetRes(data.preFilter));
			pMat->SetInput(0, 8, resMap.GetRes(data.brdfLut));
			pMat->SetOutputRT(0, resMap.GetRes(data.lighting));
			pMat->SetOutputRT(1, resMap.GetRes(data.lightingSpec));
			pMat->SetOutputRT(2, resMap.GetRes(data.lightingCopy));

			pMat->Render(pCmdList);
		});
}

NXRGPassNode<SubsurfacePassData>* Renderer::BuildSubsurfacePass(NXRGPassNode<DeferredLightingPassData>* litPassData, NXRGPassNode<GBufferPassData>* gBufferPassData)
{
	NXRGHandle pNoise64 = m_pRenderGraph->Import(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Noise2DGray_64x64));
	return m_pRenderGraph->AddPass<SubsurfacePassData>("Subsurface",
		[&](NXRGBuilder& builder, SubsurfacePassData& data) {
			data.lighting = builder.Read(litPassData->GetData().lighting);
			data.lightingSpec = builder.Read(litPassData->GetData().lightingSpec);
			data.gbuffer1 = builder.Read(gBufferPassData->GetData().rt1);
			data.noise64 = builder.Read(pNoise64);
			data.depth = builder.ReadWrite(gBufferPassData->GetData().depth);
			data.buf = builder.ReadWrite(litPassData->GetData().lightingCopy);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, SubsurfacePassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("Subsurface"));
			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 3, &NXResourceManager::GetInstance()->GetMaterialManager()->GetCBufferDiffuseProfile());

			pMat->SetInput(0, 0, resMap.GetRes(data.lighting));
			pMat->SetInput(0, 1, resMap.GetRes(data.lightingSpec));
			pMat->SetInput(0, 3, resMap.GetRes(data.gbuffer1));
			pMat->SetInput(0, 4, resMap.GetRes(data.depth));
			pMat->SetInput(0, 5, resMap.GetRes(data.noise64));
			pMat->SetOutputRT(0, resMap.GetRes(data.buf));
			pMat->SetOutputDS(resMap.GetRes(data.depth));

			pMat->Render(pCmdList);
		});
}

NXRGPassNode<SkyLightingPassData>* Renderer::BuildSkyLightingPass(NXRGPassNode<SubsurfacePassData>* sssPassData, NXRGPassNode<GBufferPassData>* gBufferPassData, NXRGHandle pCubeMap)
{
	return m_pRenderGraph->AddPass<SkyLightingPassData>("SkyLighting",
		[&](NXRGBuilder& builder, SkyLightingPassData& data) {
			data.cubeMap = builder.Read(pCubeMap);
			data.buf = builder.ReadWrite(sssPassData->GetData().buf);
			data.depth = builder.ReadWrite(gBufferPassData->GetData().depth);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, SkyLightingPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("SkyLighting"));
			pMat->SetConstantBuffer(0, 0, &m_scene->GetCubeMap()->GetCBObjectParams());
			pMat->SetConstantBuffer(0, 1, &m_scene->GetCubeMap()->GetCBDataParams());
			pMat->SetInput(0, 0, resMap.GetRes(data.cubeMap));
			pMat->SetOutputRT(0, resMap.GetRes(data.buf));
			pMat->SetOutputDS(resMap.GetRes(data.depth));

			pMat->Render(pCmdList);
		});
}

// =====================================================
// Post Processing Passes
// =====================================================

NXRGPassNode<PostProcessingPassData>* Renderer::BuildPostProcessingPass(NXRGPassNode<SkyLightingPassData>* skyPassData)
{
	NXRGHandle pPostProcess = m_pRenderGraph->Create("PostProcessing RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	return m_pRenderGraph->AddPass<PostProcessingPassData>("PostProcessing",
		[&](NXRGBuilder& builder, PostProcessingPassData& data) {
			data.skyBuf = builder.Read(skyPassData->GetData().buf);
			data.out = builder.Write(pPostProcess);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PostProcessingPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("PostProcessing"));

			m_cbColorMappingData.x = m_bEnablePostProcessing ? 1.0f : 0.0f;
			m_cbColorMapping.Update(m_cbColorMappingData);

			pMat->SetConstantBuffer(0, 2, &m_cbColorMapping);
			pMat->SetInput(0, 0, resMap.GetRes(data.skyBuf));
			pMat->SetOutputRT(0, resMap.GetRes(data.out));

			pMat->Render(pCmdList);
		});
}

NXRGPassNode<DebugLayerPassData>* Renderer::BuildDebugLayerPass(NXRGPassNode<PostProcessingPassData>* postProcessPassData, NXRGPassNode<ShadowMapPassData>* shadowMapPassData)
{
	NXRGHandle pDebugLayer = m_pRenderGraph->Create("Debug Layer RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

	return m_pRenderGraph->AddPass<DebugLayerPassData>("DebugLayer",
		[&](NXRGBuilder& builder, DebugLayerPassData& data) {
			data.postProcessOut = builder.Read(postProcessPassData->GetData().out);
			data.csmDepth = builder.Read(shadowMapPassData->GetData().csmDepth);
			data.out = builder.Write(pDebugLayer);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, DebugLayerPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("DebugLayer"));

			m_cbDebugLayerData.x = (float)m_bEnableShadowMapDebugLayer;
			m_cbDebugLayerData.y = m_fShadowMapZoomScale;
			m_cbDebugLayer.Update(m_cbDebugLayerData);

			pMat->SetConstantBuffer(0, 1, &g_cbCamera);
			pMat->SetConstantBuffer(0, 2, &m_cbDebugLayer);
			pMat->SetInput(0, 0, resMap.GetRes(data.postProcessOut));
			pMat->SetInput(0, 1, resMap.GetRes(data.csmDepth));
			pMat->SetOutputRT(0, resMap.GetRes(data.out));

			pMat->Render(pCmdList);
		});
}

NXRGPassNode<GizmosPassData>* Renderer::BuildGizmosPass(NXRGPassNode<DebugLayerPassData>* debugLayerPassData, NXRGPassNode<PostProcessingPassData>* postProcessPassData)
{
	return m_pRenderGraph->AddPass<GizmosPassData>("Gizmos",
		[&](NXRGBuilder& builder, GizmosPassData& data) {
			NXRGHandle pOut = m_bEnableDebugLayer ? debugLayerPassData->GetData().out : postProcessPassData->GetData().out;
			data.out = builder.ReadWrite(pOut);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, GizmosPassData& data) {
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
				if (pEditObj->GetVisible())
				{
					pEditObj->Update(pCmdList);

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
}

void Renderer::BuildFinalQuadPass(NXRGPassNode<GizmosPassData>* gizmosPassData)
{
	NXRGHandle pFinalRT = m_pRenderGraph->Import(m_pFinalRT);
	m_pRenderGraph->AddPass<FinalQuadPassData>("FinalQuad",
		[&](NXRGBuilder& builder, FinalQuadPassData& data) {
			data.gizmosOut = builder.Read(gizmosPassData->GetData().out);
			data.finalOut = builder.Write(pFinalRT);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, FinalQuadPassData& data) {
			auto pMat = static_cast<NXGraphicPassMaterial*>(NXPassMng->GetPassMaterial("FinalQuad"));
			pMat->SetInput(0, 0, resMap.GetRes(data.gizmosOut));
			pMat->SetOutputRT(0, resMap.GetRes(data.finalOut));

			pMat->Render(pCmdList);
		});
}
