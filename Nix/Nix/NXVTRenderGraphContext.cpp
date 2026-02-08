#include "NXVirtualTexture.h"
#include "NXRenderGraph.h"
#include "NXTerrainLODStreamer.h"
#include "RenderGraphPassData.h"
#include "NXPassMaterial.h"
#include "NXPassMaterialManager.h"

void NXVirtualTexture::RegisterClearSector2VirtImgPass()
{
	m_ctx.pRG->AddPass<Sector2VirtImgClearPassData>("Sector2VirtImg Clear",
		[&](NXRGBuilder& builder, Sector2VirtImgClearPassData& data)
		{
			data.Sector2VirtImg = builder.Write(m_ctx.hSector2VirtImg);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, Sector2VirtImgClearPassData& data)
		{
			auto pTex = resMap.GetRes(data.Sector2VirtImg).As<NXTexture2D>();
			pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			UINT clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pTex->GetUAV(0);
			NXShVisDescHeap->PushFluid(cpuHandle);
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
			pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pTex->GetD3DResource(), clearValues, 0, nullptr);
		});
}

void NXVirtualTexture::RegisterClearIndirectTexturePass()
{
	m_ctx.pRG->AddPass<IndirectTextureClearPassData>("IndirectTexture Clear",
		[&](NXRGBuilder& builder, IndirectTextureClearPassData& data)
		{
			data.IndirectTexture = builder.Write(m_ctx.hIndirectTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, IndirectTextureClearPassData& data)
		{
			auto pTex = resMap.GetRes(data.IndirectTexture).As<NXTexture2D>();
			pTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			UINT clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
			int mipLevels = pTex->GetMipLevels();
			for (int i = 0; i < mipLevels; i++)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pTex->GetUAV(i);
				NXShVisDescHeap->PushFluid(cpuHandle);
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();
				pCmdList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pTex->GetD3DResource(), clearValues, 0, nullptr);
			}
		});
}

void NXVirtualTexture::RegisterUpdateSector2VirtImgPass()
{
	if (m_enableDebugPrint)
	{
		for (auto& data : m_cbDataSector2VirtImg)
		{
			//indiTexData(((indiTexPos.x & 0xFFF) << 20) | ((indiTexPos.y & 0xFFF) << 8) | (std::countr_zero((uint32_t)indiTexSize) & 0xFF)) // std::countr_zero = log2 of POTsize;
			int indiTexPosX = (data.indiTexData >> 20) & 0xFFF;
			int indiTexPosY = (data.indiTexData >> 8) & 0xFFF;
			int indiTexSize = 1 << (data.indiTexData & 0xFF);

			printf("[UpdateSector2VirtImg] sectorPos: (%d, %d), indiTexPos: (%d, %d), indiTexSize: %d\n",
				data.sectorPos.x, data.sectorPos.y,
				indiTexPosX, indiTexPosY,
				indiTexSize);
		}
	}

	m_ctx.pRG->AddPass<Sector2VirtImgPassData>("UpdateSector2VirtImg",
		[&](NXRGBuilder& builder, Sector2VirtImgPassData& data)
		{
			data.Sector2VirtImg = builder.Write(m_ctx.hSector2VirtImg);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, Sector2VirtImgPassData& data)
		{
			uint32_t threadGroups = (m_cbDataSector2VirtImg.size() + 7) / 8;
			if (threadGroups == 0)
				return;

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("UpdateSector2VirtImg"));
			pMat->SetOutput(0, 0, resMap.GetRes(data.Sector2VirtImg));
			pMat->SetConstantBuffer(0, 0, &m_cbSector2VirtImg);
			pMat->SetConstantBuffer(0, 1, &m_cbSector2VirtImgNum);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);
			pCmdList->Dispatch(threadGroups, 1, 1);
		});

}

void NXVirtualTexture::RegisterBakePhysicalPagePass()
{
	if (m_enableDebugPrint)
	{
		for (auto& data : m_cbDataPhysPageBake)
		{
			printf("[BakePhysicalPage] SectorID: (%d, %d), PageID: (%d, %d), GPU Mip: %d, IndiTexLog2Size: %d\n", data.sector.x, data.sector.y, data.pageID.x, data.pageID.y, data.gpuMip, data.indiTexLog2Size);
		}
	}

	if (m_cbDataPhysPageBake.empty())
		return;

	auto& pStreamingData = m_ctx.pTerrainLODStreamer->GetStreamingData();

	m_ctx.pRG->AddPass<PhysicalPageBakerPassData>("PhysicalPageBaker",
		[&](NXRGBuilder& builder, PhysicalPageBakerPassData& data)
		{
			data.Sector2NodeIDTex = builder.Read(m_ctx.hSector2NodeIDTex);
			data.SplatMapAtlas = builder.Read(m_ctx.hSplatMapAtlas);
			data.AlbedoMapArray = builder.Read(m_ctx.hAlbedoMapArray);
			data.NormalMapArray = builder.Read(m_ctx.hNormalMapArray);
			data.PhysicalPageAlbedo = builder.Read(m_ctx.hAlbedoPhysicalPage);
			data.PhysicalPageNormal = builder.Read(m_ctx.hNormalPhysicalPage);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, PhysicalPageBakerPassData& data)
		{
			uint32_t threadNum = (g_virtualTextureConfig.PhysicalPageTileSize + 7) / 8;
			uint32_t bakeTexNum = m_cbDataPhysPageBake.size();

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("PhysicalPageBaker"));
			pMat->SetConstantBuffer(0, 0, &m_cbPhysPageBake);
			pMat->SetConstantBuffer(0, 1, &pStreamingData.GetNodeDescArray());
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
}

void NXVirtualTexture::RegisterUpdateIndirectTexturePass()
{
	if (m_enableDebugPrint)
	{
		for (auto& data : m_cbDataUpdateIndex)
		{
			printf("[UpdateIndirectTexture] PageID: (%d, %d), gpumip: %d, index: %d\n", data.pageID.x, data.pageID.y, data.mip, data.index);
		}
	}

	if (m_cbDataUpdateIndex.empty())
		return;

	m_ctx.pRG->AddPass<UpdateIndirectTexturePassData>("UpdateIndirectTexture",
		[&](NXRGBuilder& builder, UpdateIndirectTexturePassData& data)
		{
			data.IndirectTexture = builder.Write(m_ctx.hIndirectTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, UpdateIndirectTexturePassData& data)
		{
			uint32_t bakeTexNum = m_cbDataUpdateIndex.size();

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("UpdateIndirectTexture"));
			pMat->SetConstantBuffer(0, 0, &m_cbUpdateIndex);

			int mips = 11;
			for (int i = 0; i < mips; i++)
				pMat->SetOutput(0, i, resMap.GetRes(data.IndirectTexture), i);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(bakeTexNum, 1, 1);
		});
}

void NXVirtualTexture::RegisterRemoveIndirectTextureSectorPass(const NXConstantBuffer<CBufferRemoveSector>& pCBRemoveSector, const CBufferRemoveSector& removeData)
{
	if (m_enableDebugPrint)
	{
		printf("RemoveIndirectTextureSectors: ImagePos: (%d, %d), ImageSize: %d, MaxRemoveMip: %d\n", removeData.imagePos.x, removeData.imagePos.y, removeData.imageSize, removeData.maxRemoveMip);
	}

	m_ctx.pRG->AddPass<RemoveIndirectTextureSectorPassData>("RemoveIndirectTextureSectors",
		[&](NXRGBuilder& builder, RemoveIndirectTextureSectorPassData& data)
		{
			data.IndirectTexture = builder.Write(m_ctx.hIndirectTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, RemoveIndirectTextureSectorPassData& data)
		{
			int thdGroupCount = (removeData.imageSize + 7) / 8;

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("RemoveIndirectTextureSectors"));
			pMat->SetConstantBuffer(0, 0, &pCBRemoveSector);

			int mips = 11;
			for (int i = 0; i < mips; i++)
				pMat->SetOutput(0, i, resMap.GetRes(data.IndirectTexture), i);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(thdGroupCount, thdGroupCount, 1);
		});
}

void NXVirtualTexture::RegisterMigrateIndirectTextureSectorPass(const NXConstantBuffer<CBufferMigrateSector>& pCBMigrateSector, const CBufferMigrateSector& migrateData)
{
	if (m_enableDebugPrint)
	{
		printf("MigrateIndirectTextureSectors: FromImagePos: (%d, %d), ToImagePos: (%d, %d), FromImageSize: %d, ToImageSize: %d, MipDelta: %d\n",
			migrateData.fromImagePos.x, migrateData.fromImagePos.y,
			migrateData.toImagePos.x, migrateData.toImagePos.y,
			migrateData.fromImageSize, migrateData.toImageSize,
			migrateData.mipDelta);
	}

	m_ctx.pRG->AddPass<MigrateIndirectTextureSectorPassData>("MigrateIndirectTextureSectors",
		[&](NXRGBuilder& builder, MigrateIndirectTextureSectorPassData& data)
		{
			data.IndirectTexture = builder.Write(m_ctx.hIndirectTexture);
		},
		[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, MigrateIndirectTextureSectorPassData& data)
		{
			int thdGroupCount = (migrateData.fromImageSize + 7) / 8;

			auto pMat = static_cast<NXComputePassMaterial*>(NXPassMng->GetPassMaterial("MigrateIndirectTextureSectors"));
			pMat->SetConstantBuffer(0, 0, &pCBMigrateSector);

			int mips = 11;
			for (int i = 0; i < mips; i++)
				pMat->SetOutput(0, i, resMap.GetRes(data.IndirectTexture), i);

			pMat->RenderSetTargetAndState(pCmdList);
			pMat->RenderBefore(pCmdList);

			pCmdList->Dispatch(thdGroupCount, thdGroupCount, 1);
		});
}
