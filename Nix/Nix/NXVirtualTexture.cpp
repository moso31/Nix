#include "NXVirtualTexture.h"
#include "NXVTDebugger.h"
#include "NXResourceManager.h"
#include "NXCamera.h"
#include "NXTexture.h"
#include "NXTerrainLODStreamer.h"
#include <unordered_set>

NXVirtualTexture::NXVirtualTexture(class NXCamera* pCam, class NXTerrainLODStreamer* pTerrainLODStreamer) :
	m_pCamera(pCam),
	m_pTerrainLODStreamer(pTerrainLODStreamer),
	m_vtSectorLodDists({ 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f }),
	m_vtSectorLodMaxDist(400),
	m_lruCache(g_virtualTextureConfig.PhysicalPageTileNum)
{
	m_pVirtImageQuadTree = new NXVTImageQuadTree();

	m_pSector2VirtImg = NXManager_Tex->CreateTexture2D("VirtualTexture_Sector2VirtImg", DXGI_FORMAT_R32_UINT, VT_SECTOR2VIRTIMG_SIZE, VT_SECTOR2VIRTIMG_SIZE, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pSector2VirtImg->SetViews(1, 0, 0, 1);
	m_pSector2VirtImg->SetSRV(0);
	m_pSector2VirtImg->SetUAV(0);

	m_pPhysicalPageAlbedo = NXManager_Tex->CreateTexture2DArray("VirtualTexture_PhysicalPage_Albedo", DXGI_FORMAT_R8G8B8A8_UNORM, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileNum, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pPhysicalPageNormal = NXManager_Tex->CreateTexture2DArray("VirtualTexture_PhysicalPage_Normal", DXGI_FORMAT_R8G8B8A8_UNORM, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileNum, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	int mip = 11;
	m_pIndirectTexture = NXManager_Tex->CreateTexture2D("VirtualTexture_IndirectTexture", DXGI_FORMAT_R16_UINT, g_virtualTextureConfig.IndirectTextureSize, g_virtualTextureConfig.IndirectTextureSize, mip, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pIndirectTexture->SetViews(1, 0, 0, mip);
	m_pIndirectTexture->SetSRV(0);
	for (int i = 0; i < mip; i++)
		m_pIndirectTexture->SetUAV(i, i);

	m_cbSector2VirtImg.Recreate(CB_SECTOR2VIRTIMG_DATA_NUM);
	m_cbPhysPageBake.Recreate(BAKE_PHYSICAL_PAGE_PER_FRAME);
	m_cbUpdateIndex.Recreate(UPDATE_INDIRECT_TEXTURE_PER_FRAME);
	m_physPageSlotSectorVersion.assign(g_virtualTextureConfig.PhysicalPageTileNum, 0);

	m_vtReadbackData = new NXReadbackData("VT Readback CPUdata");
}

NXVirtualTexture::~NXVirtualTexture()
{
	Release();
}

void NXVirtualTexture::Release()
{
	if (m_pVirtImageQuadTree)
	{
		delete m_pVirtImageQuadTree;
		m_pVirtImageQuadTree = nullptr;
	}
}

void NXVirtualTexture::Update()
{
	// 仅首帧执行: Sector2VirtImg IndirectTexture 全像素初始化-1
	if (m_bNeedClearSector2VirtImg)
	{
		RegisterClearSector2VirtImgPass();
		m_bNeedClearSector2VirtImg = false;
	}

	if (m_bNeedClearIndirectTexture)
	{
		RegisterClearIndirectTexturePass();
		NXVTDebugger::GetInstance().TrackerClear();
		m_bNeedClearIndirectTexture = false;
	}

	switch (m_updateState)
	{
	case NXVTUpdateState::None:
	case NXVTUpdateState::Finish:
		m_updateState = NXVTUpdateState::Ready;
		break;
	case NXVTUpdateState::Ready:
		UpdateNearestSectors();

		RegisterUpdateSector2VirtImgPass();
		m_updateState = NXVTUpdateState::WaitReadback;
		break;
	case NXVTUpdateState::WaitReadback:
		break;
	case NXVTUpdateState::Reading:
		if (m_bReadbackFinish)
		{
			BakePhysicalPages();

			RegisterBakePhysicalPagePass();
			RegisterUpdateIndirectTexturePass();
			m_updateState = NXVTUpdateState::PhysicalPageBake;
			m_bReadbackFinish = false;
		}
		break;
	case NXVTUpdateState::PhysicalPageBake:
		m_updateState = NXVTUpdateState::Finish;
		break;
	default:
		break;
	}
}

void NXVirtualTexture::UpdateCBData(const Vector2& rtSize)
{
	m_cbDataVTReadback = Vector4(rtSize.x, rtSize.y, 0.0f, 0.0f);
	m_cbVTReadback.Update(m_cbDataVTReadback);
}

void NXVirtualTexture::UpdateNearestSectors()
{
	m_lastSectors.swap(m_sectors);
	m_sectors.clear();
	m_cbDataSector2VirtImg.clear();

	Vector2 camPosXZ = m_pCamera->GetTranslation().GetXZ();

	// 获取本帧最新的sectors
	Vector2 sectorPosXZ = Vector2::Floor(camPosXZ * SECTOR_SIZEF_INV);
	Int2 camSectorXZ(sectorPosXZ); 
	int sectorRange = (m_vtSectorLodMaxDist + SECTOR_SIZE) >> SECTOR_SIZE_LOG2;
	for (int i = camSectorXZ.x - sectorRange; i <= camSectorXZ.x + sectorRange; i++)
	{
		for (int j = camSectorXZ.y - sectorRange; j <= camSectorXZ.y + sectorRange; j++)
		{
			Int2 sectorID(i, j);
			float dist2 = GetDist2OfSectorToCamera(camPosXZ, sectorID * SECTOR_SIZE);
			if (dist2 < m_vtSectorLodMaxDist * m_vtSectorLodMaxDist)
			{
				int size = VTIMAGE_MAX_NODE_SIZE >> GetVTImageSizeFromDist2(dist2);

				// 保持这里push的sectorID是升序的，不然两帧关系比对会出错
				m_sectors.push_back(NXVTSector(sectorID, size));
			}
		}
	}

	// 两帧关系比对
	std::vector<NXVTSector> createSector; // 本帧创建的
	std::vector<NXVTSector> removeSector; // 本帧移除的
	std::vector<NXVTChangeSector> changeSector; // 本帧升级/降级的
	m_cbDataRemoveSector.clear();
	m_cbDataMigrateRemoveSector.clear();
	m_cbDataMigrateSector.clear();
	std::vector<Int2> migrateSectorIDs;
	int i = 0;
	int j = 0;
	while (i < m_lastSectors.size() && j < m_sectors.size())
	{
		auto& A = m_lastSectors[i];
		auto& B = m_sectors[j];

		if (A.id == B.id)
		{
			if (A.imageSize != B.imageSize)
			{
				NXVTChangeSector cs;
				cs.oldData = A;
				cs.changedImageSize = B.imageSize;
				changeSector.push_back(cs);
			}
			else
			{
				// keepSector. do nothing
			}
			i++;
			j++;
		}
		else if (A.Less(B)) // A < B
		{
			removeSector.push_back(A);
			i++;
		}
		else // A > B
		{
			createSector.push_back(B);
			j++;
		}
	}

	while (i < m_lastSectors.size()) { removeSector.push_back(m_lastSectors[i]); i++; }
	while (j < m_sectors.size()) { createSector.push_back(m_sectors[j]); j++; }

	// 准备更新 Sector2VirtImg
	for (auto& s : createSector)
	{
		Int2 virtImgPos = m_pVirtImageQuadTree->Alloc(s.imageSize, s.id);
		assert(virtImgPos.x >= 0 && virtImgPos.y >= 0);
		m_sector2VirtImagePos[s] = virtImgPos;
		m_cbDataSector2VirtImg.push_back({ s.id - g_terrainConfig.MinSectorID, virtImgPos, s.imageSize });
	}

	for (auto& s : removeSector)
	{
		if (m_sector2VirtImagePos.find(s) != m_sector2VirtImagePos.end())
		{
			Int2 virtImgPos = m_sector2VirtImagePos[s];
			m_pVirtImageQuadTree->Free(virtImgPos, s.imageSize);
			m_sector2VirtImagePos.erase(s);
			m_cbDataSector2VirtImg.push_back({ s.id - g_terrainConfig.MinSectorID, -1 });

			CBufferRemoveSector removeData;
			removeData.imagePos = virtImgPos;
			removeData.imageSize = s.imageSize;
			removeData.maxRemoveMip = 114514; // 移除所有Mip，够大就行
			m_cbDataRemoveSector.push_back(removeData);
		}
		else
		{
			NXVTDebugger::GetInstance().Log(NXVTDebugCategory::SectorUpdate, "WARNING: removeSector not found!\n");
		}
	}

	for (auto& s : changeSector)
	{
		Int2 newVirtImgPos = m_pVirtImageQuadTree->Alloc(s.changedImageSize, s.oldData.id);
		assert(newVirtImgPos.x >= 0 && newVirtImgPos.y >= 0);
		NXVTSector newSector = s.oldData;
		newSector.imageSize = s.changedImageSize;
		m_sector2VirtImagePos[newSector] = newVirtImgPos;
		m_cbDataSector2VirtImg.push_back({ s.oldData.id - g_terrainConfig.MinSectorID, newVirtImgPos, s.changedImageSize });

		if (m_sector2VirtImagePos.find(s.oldData) != m_sector2VirtImagePos.end())
		{
			Int2 oldVirtImgPos = m_sector2VirtImagePos[s.oldData];
			m_pVirtImageQuadTree->Free(oldVirtImgPos, s.oldData.imageSize);
			m_sector2VirtImagePos.erase(s.oldData);
			//m_cbDataSector2VirtImg.push_back({ s.oldData.id - g_terrainConfig.MinSectorID, -1 });

			CBufferMigrateSector migrateData;
			migrateData.fromImagePos = oldVirtImgPos;
			migrateData.toImagePos = newVirtImgPos;
			migrateData.fromImageSize = s.oldData.imageSize;
			migrateData.toImageSize = s.changedImageSize;
			auto& A = migrateData.fromImageSize;
			auto& B = migrateData.toImageSize;
			migrateData.mipDelta = std::countr_zero((uint32_t)(std::max(A, B) / std::min(A, B))); // 例如log2(from:32/to:8)=2，即存在2级mip差
			m_cbDataMigrateSector.push_back(migrateData);
			migrateSectorIDs.push_back(s.oldData.id);
		}
		else
		{
			NXVTDebugger::GetInstance().Log(NXVTDebugCategory::SectorUpdate, "WARNING: changeSector old sector not found!\n");
		}
	}

	if (m_cbDataSector2VirtImg.size() >= 256)
	{
		NXVTDebugger::GetInstance().Log(NXVTDebugCategory::SectorUpdate, "WARNING: Sector2VirtImg data >= 256\n");
	}

	for (int i = 0; i < m_cbDataRemoveSector.size(); i++)
	{
		// 页表删除
		auto& removeData = m_cbDataRemoveSector[i];
		m_cbArrayRemoveSector[i].Update(removeData);
		RegisterRemoveIndirectTextureSectorPass(m_cbArrayRemoveSector[i], removeData);
		NXVTDebugger::GetInstance().TrackerSimulateRemove(removeData);
	}

	for (int i = 0; i < m_cbDataMigrateSector.size(); i++)
	{
		// 页表迁移（升/降 sector）
		auto& migrateData = m_cbDataMigrateSector[i];
		m_cbArrayMigrateSector[i].Update(migrateData);
		RegisterMigrateIndirectTextureSectorPass(m_cbArrayMigrateSector[i], migrateData);
		NXVTDebugger::GetInstance().TrackerSimulateMigrate(migrateData);

		// 降采样，大图换小图，大图的前mip级页表 也需要完全清空
		if (migrateData.fromImageSize > migrateData.toImageSize)
		{
			CBufferRemoveSector migrateRemoveData;
			migrateRemoveData.imagePos = migrateData.fromImagePos;
			migrateRemoveData.imageSize = migrateData.fromImageSize;
			migrateRemoveData.maxRemoveMip = migrateData.mipDelta; // 清空前N级mip
			m_cbDataMigrateRemoveSector.push_back(migrateRemoveData);
		}

		// 升采样不需要清空，本来就是被清空过的才会被作为migrate目标
		// 测试代码：升采样清空
		// if (migrateData.fromImageSize < migrateData.toImageSize)
		// {
		// 	CBufferRemoveSector migrateRemoveData;
		// 	migrateRemoveData.imagePos = migrateData.toImagePos;
		// 	migrateRemoveData.imageSize = migrateData.toImageSize;
		// 	migrateRemoveData.maxRemoveMip = migrateData.mipDelta; 
		// 	m_cbDataMigrateRemoveSector.push_back(migrateRemoveData);
		// }
	}

	for (int i = 0; i < m_cbDataMigrateRemoveSector.size(); i++)
	{
		auto& migrateRemoveData = m_cbDataMigrateRemoveSector[i];
		m_cbArrayMigrateRemoveSector[i].Update(migrateRemoveData);
		RegisterRemoveIndirectTextureSectorPass(m_cbArrayMigrateRemoveSector[i], migrateRemoveData);
		NXVTDebugger::GetInstance().TrackerSimulateRemove(migrateRemoveData);
	}

	// 问题：
	// 页表迁移（升降采样）的时候，对应sector的所有有效像素都会平移到另一个区域，所以LRU记录的pageID也得跟着变
	// 以前由于没有考虑到这点，迁移sector后，LRU的pageID、mip还是旧的
	// 然后这个sector就会随着迁移到处流转，LRU也会持续迭代
	// 一旦后续：
	// 1. 有一帧LRU把这个{pageID、mip}剔除了，换上新的pageID
	// 2. 恰好赶上对应sector一直在migrate，也没被移除
	// 3. 加上此时镜头突然拉近，GBuffer就会读页表-结果拿到了已经有问题的sector-导致渲染错乱。
	// 这三重因素叠加下就会导致渲染错乱，但该bug只闪烁一帧就会被修复，因为下一轮状态readback发现这个pageID实际并不在LRU中，很快就会发起申请新页。
	// 解决方案：
	// 如下，在sector迁移的时候，同步检查所有LRU key并进行维护。
	for (int mi = 0; mi < m_cbDataMigrateSector.size(); mi++)
	{
		auto& migrateData = m_cbDataMigrateSector[mi];
		Int2 sectorID = migrateSectorIDs[mi];
		Int2 fromBase(migrateData.fromImagePos.x * migrateData.fromImageSize, migrateData.fromImagePos.y * migrateData.fromImageSize);
		Int2 toBase(migrateData.toImagePos.x * migrateData.toImageSize, migrateData.toImagePos.y * migrateData.toImageSize);
		int newLog2Size = std::countr_zero((uint32_t)migrateData.toImageSize);

		std::vector<std::pair<uint64_t, uint64_t>> replacements;
		for (auto& keyHash : m_lruCache.GetKeys()) // 遍历所有key
		{
			NXVTLRUKey key(keyHash);
			if (key.sector != sectorID) continue; // 和当前sector没关系就跳

			// LRU上现在记录的pageID和mip
			int gpuMip = key.gpuMip;
			Int2 pageID = key.pageID;
			
			if (migrateData.fromImageSize < migrateData.toImageSize) 
			{
				// upscale: fromMip = gpuMip, toMip = gpuMip + mipDelta
				int fromMip = gpuMip;
				int size = migrateData.fromImageSize >> fromMip;
				if (size <= 0) continue;

				// LRU的位置-迁移起始位置，判断LRU的{page}是否在本次迁移的起始范围内
				Int2 coord(pageID.x - (fromBase.x >> fromMip), pageID.y - (fromBase.y >> fromMip));
				if (coord.x < 0 || coord.x >= size || coord.y < 0 || coord.y >= size) continue; // 如果page不在本次迁移的起始范围内，说明没影响，跳过

				// 能走到这里说明页表中有像素受本次迁移影响！得维护LRU了
				// 把迁移后的pageID位置和mip信息算出来
				int newGpuMip = gpuMip + migrateData.mipDelta;
				Int2 newPageID((toBase.x >> newGpuMip) + coord.x, (toBase.y >> newGpuMip) + coord.y);

				// 记录要替换的新旧key pair.
				NXVTLRUKey newKey;
				newKey.sector = sectorID;
				newKey.pageID = newPageID;
				newKey.gpuMip = newGpuMip;
				newKey.indiTexLog2Size = newLog2Size;
				replacements.push_back({ keyHash, newKey.GetKey() });
			}
			else
			{
				// downscale: fromMip = gpuMip, toMip = gpuMip - mipDelta
				if (gpuMip < migrateData.mipDelta) continue;

				int toMip = gpuMip - migrateData.mipDelta;
				int size = migrateData.toImageSize >> toMip;
				if (size <= 0) continue;

				Int2 coord(pageID.x - (fromBase.x >> gpuMip), pageID.y - (fromBase.y >> gpuMip));
				if (coord.x < 0 || coord.x >= size || coord.y < 0 || coord.y >= size) continue;

				int newGpuMip = toMip;
				Int2 newPageID((toBase.x >> newGpuMip) + coord.x, (toBase.y >> newGpuMip) + coord.y);

				NXVTLRUKey newKey;
				newKey.sector = sectorID;
				newKey.pageID = newPageID;
				newKey.gpuMip = newGpuMip;
				newKey.indiTexLog2Size = newLog2Size;
				replacements.push_back({ keyHash, newKey.GetKey() });
			}
		}

		// 实际替换LRU key。
		for (auto& [oldHash, newHash] : replacements)
		{
			m_lruCache.ReplaceKey(oldHash, newHash);
		}
	}

	// 准备更新 Sector2VirtImg
	m_cbSector2VirtImg.Update(m_cbDataSector2VirtImg);
	m_cbSector2VirtImgNum.Update((int)m_cbDataSector2VirtImg.size());
}

void NXVirtualTexture::BakePhysicalPages()
{
	if (m_vtReadbackData.IsNull())
		return;

	auto pVTReadbackData = m_vtReadbackData->Clone();
	if (pVTReadbackData.empty())
		return;

	m_cbDataPhysPageBake.clear();
	m_cbDataUpdateIndex.clear();

	std::unordered_set<uint32_t> readbackSets;
	const uint32_t* readbackData = reinterpret_cast<const uint32_t*>(pVTReadbackData.data());
	uint32_t readbackDataNum = m_vtReadbackData->GetWidth();
	for (int i = 0; i < readbackDataNum; i++)
	{
		// readbackData: xy = pageID, z = gpu mip, w = log2indiTexSize;
		auto& data = readbackData[i];
		if (data == 0xFFFFFFFF) continue;
		readbackSets.insert(data);
	}

	std::vector<uint32_t> requeue;
	auto& sectorVersionMap = m_pTerrainLODStreamer->GetSectorVersionMap();
	int lruInsertNum = 0;
	for (auto& data : readbackSets)
	{
		Int2 pageID((data >> 20) & 0xFFF, (data >> 8) & 0xFFF);
		uint32_t gpuMip = (data >> 4) & 0xF;
		uint32_t log2IndiTexSize = (data >> 0) & 0xF;
		Int2 pageIDMip0 = pageID << gpuMip;

		Int2 sectorID = m_pVirtImageQuadTree->GetSector(pageIDMip0, log2IndiTexSize);
		if (sectorID != Int2(INT_MIN))
		{
			NXVTLRUKey key;
			key.sector = sectorID;
			key.pageID = pageID;
			key.gpuMip = gpuMip;
			key.indiTexLog2Size = log2IndiTexSize;

			uint32_t keyVersion = sectorVersionMap.GetVersion(sectorID - g_terrainConfig.MinSectorID);

			uint64_t keyHash = key.GetKey();
			uint64_t oldKeyHash = -1;
			if (m_lruCache.Find(keyHash))
			{
				int cacheIdx = m_lruCache.Touch(keyHash);
				key.bakePhysicalPageIndex = cacheIdx;

				uint32_t oldKeyVersion = m_physPageSlotSectorVersion[cacheIdx];
				if (keyVersion != oldKeyVersion)
				{
					m_physPageSlotSectorVersion[cacheIdx] = keyVersion;
					m_cbDataPhysPageBake.push_back(key);

					NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "LRU Update: %d, Sector: (%d, %d), PageID: (%d, %d), GPU Mip: %d, IndiTexLog2Size: %d, cacheIdx: %d\n", data, key.sector.x, key.sector.y, key.pageID.x, key.pageID.y, key.gpuMip, key.indiTexLog2Size, cacheIdx);
				}

				m_cbDataUpdateIndex.push_back(CBufferPhysPageUpdateIndex(cacheIdx, pageID, gpuMip));
				lruInsertNum++;
			}
			else
			{
				int cacheIdx = m_lruCache.Insert(keyHash, oldKeyHash);
				key.bakePhysicalPageIndex = cacheIdx;

				if (oldKeyHash < UINT64_MAX - g_virtualTextureConfig.PhysicalPageTileNum)
				{
					uint32_t removePage = oldKeyHash & 0xFFFFFFFF; // LRUKey的后32位 和pageIDTexture的格式完全一致
					Int2 pageID((removePage >> 20) & 0xFFF, (removePage >> 8) & 0xFFF);
					uint32_t gpuMip = (removePage >> 4) & 0xF;
					uint32_t log2IndiTexSize = (removePage >> 0) & 0xF;

					if (!readbackSets.contains(removePage))
					{
						m_cbDataUpdateIndex.push_back(CBufferPhysPageUpdateIndex(-1, pageID, gpuMip));

						NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "LRU Replace: cacheIdx: %d, old: PageID(%d,%d) Mip%d Log2Size%d -> new: Sector(%d,%d) PageID(%d,%d) Mip%d Log2Size%d\n",
							cacheIdx,
							pageID.x, pageID.y, gpuMip, log2IndiTexSize,
							key.sector.x, key.sector.y, key.pageID.x, key.pageID.y, key.gpuMip, key.indiTexLog2Size);
					}

					lruInsertNum++;
				}

				m_cbDataPhysPageBake.push_back(key);
				m_cbDataUpdateIndex.push_back(CBufferPhysPageUpdateIndex(cacheIdx, pageID, gpuMip));
				lruInsertNum++;

				NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "LRU Insert: %d, Sector: (%d, %d), PageID: (%d, %d), GPU Mip: %d, IndiTexLog2Size: %d, cacheIdx: %d\n", data, key.sector.x, key.sector.y, key.pageID.x, key.pageID.y, key.gpuMip, key.indiTexLog2Size, cacheIdx);
			}

			if (lruInsertNum >= UPDATE_INDIRECT_TEXTURE_PER_FRAME)
			{
				NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "WARNING: UPDATE_INDIRECT_TEXTURE_PER_FRAME\n");
				break;
			}

			if (m_cbDataPhysPageBake.size() >= BAKE_PHYSICAL_PAGE_PER_FRAME)
			{
				NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "WARNING: BAKE_PHYSICAL_PAGE_PER_FRAME\n");
				break;
			}
		}
	}

	std::unordered_set<uint32_t> x;
	for (auto& cb : m_cbDataUpdateIndex)
	{
		uint32_t test = (cb.pageID.x << 20) | (cb.pageID.y << 8) | (cb.mip << 4);
		if (x.contains(test))
		{
			NXVTDebugger::GetInstance().Log(NXVTDebugCategory::LRUCache, "WARNING: page existed!\n");
			break;
		}
		x.insert(test);
	}

	// Tracker: simulate per-pixel IndirectTexture updates on CPU side
	for (auto& cb : m_cbDataUpdateIndex)
	{
		NXVTDebugger::GetInstance().TrackerSimulateUpdateIndex(cb.index, cb.pageID, cb.mip);
	}

	m_cbPhysPageBake.Update(m_cbDataPhysPageBake);
	m_cbUpdateIndex.Update(m_cbDataUpdateIndex);
}

float NXVirtualTexture::GetDist2OfSectorToCamera(const Vector2& camPos, const Int2& sectorCorner)
{
	Vector2 sectorMin(sectorCorner);
	Vector2 sectorMax(sectorCorner + SECTOR_SIZE);

	float dx = 0.0f;
	float dy = 0.0f;
	if (camPos.x < sectorMin.x) dx = sectorMin.x - camPos.x;
	else if (camPos.x > sectorMax.x) dx = camPos.x - sectorMax.x;
	if (camPos.y < sectorMin.y) dy = sectorMin.y - camPos.y;
	else if (camPos.y > sectorMax.y) dy = camPos.y - sectorMax.y;

	return dx * dx + dy * dy;
}

int NXVirtualTexture::GetVTImageSizeFromDist2(const float dist2)
{
	for (int i = 0; i < VTSECTOR_LOD_NUM; i++)
	{
		float lodDist = m_vtSectorLodDists[i];
		if (dist2 < lodDist * lodDist) return i;
	}
	return VTSECTOR_LOD_NUM - 1;
}
