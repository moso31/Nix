#include "NXVirtualTexture.h"
#include "NXResourceManager.h"
#include "NXCamera.h"
#include "NXTexture.h"
#include <unordered_set>

NXVirtualTexture::NXVirtualTexture(class NXCamera* pCam) :
	m_pCamera(pCam),
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
	m_pIndirectTexture = NXManager_Tex->CreateTexture2D("VirtualTexture_IndirectTexture", DXGI_FORMAT_R32_UINT, g_virtualTextureConfig.IndirectTextureSize, g_virtualTextureConfig.IndirectTextureSize, mip, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pIndirectTexture->SetViews(1, 0, 0, mip);
	m_pIndirectTexture->SetSRV(0);
	for (int i = 0; i < mip; i++)
		m_pIndirectTexture->SetUAV(i, i);

	m_cbSector2VirtImg.Recreate(CB_SECTOR2VIRTIMG_DATA_NUM);
	m_cbPhysPageBake.Recreate(BAKE_PHYSICAL_PAGE_PER_FRAME);
	m_cbUpdateIndex.Recreate(BAKE_PHYSICAL_PAGE_PER_FRAME);

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
			m_updateState = NXVTUpdateState::PhysicalPageBake;
			m_bReadbackFinish = false;
			BakePhysicalPages();
			RegisterBakePhysicalPagePass();
			RegisterUpdateIndirectTexturePass();
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
	std::vector<NXVTSector> keepSector;	// 没发生任何变化的
	std::vector<NXVTChangeSector> changeSector; // 本帧升级/降级的
	int i = 0;
	int j = 0;
	while (i < m_lastSectors.size() && j < m_sectors.size())
	{
		auto& A = m_lastSectors[i];
		auto& B = m_sectors[j];

		if (A.id == B.id)
		{
			if (A.imageSize == B.imageSize)
			{
				keepSector.push_back(A);
			}
			else
			{
				NXVTChangeSector cs;
				cs.oldData = A;
				cs.changedImageSize = B.imageSize;
				changeSector.push_back(cs);
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
		}
	}

	for (auto& s : changeSector)
	{
		Int2 newVirtImgPos = m_pVirtImageQuadTree->Alloc(s.changedImageSize, s.oldData.id);
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
		}
	}

	if (m_cbDataSector2VirtImg.size() >= 256)
	{
		printf("WARNING\n");
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

	std::vector<uint64_t> pendingRemoveData;

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

			uint64_t keyHash = key.GetKey();

			uint64_t oldKeyHash = -1;
			if (m_lruCache.Find(keyHash))
			{
				m_lruCache.Touch(keyHash);
			}
			else
			{
				int cacheIdx = m_lruCache.Insert(keyHash, oldKeyHash);
				if (oldKeyHash < UINT64_MAX - g_virtualTextureConfig.PhysicalPageTileNum)	
				{
					pendingRemoveData.push_back(oldKeyHash);
					lruInsertNum++;
				}

				m_cbDataPhysPageBake.push_back(key);
				m_cbDataUpdateIndex.push_back(CBufferPhysPageUpdateIndex(cacheIdx, pageID, gpuMip));
				lruInsertNum++;

				//printf("Sector: (%d, %d), PageID: (%d, %d), GPU Mip: %d, IndiTexLog2Size: %d, cacheIdx: %d\n", key.sector.x, key.sector.y, key.pageID.x, key.pageID.y, key.gpuMip, key.indiTexLog2Size, cacheIdx);

				if (lruInsertNum >= BAKE_PHYSICAL_PAGE_PER_FRAME)
					break;
			}
		}
	}

	for (auto& removeLRUKey : pendingRemoveData)
	{
		uint32_t removePage = removeLRUKey & 0xFFFFFFFF; // LRUKey的后32位 和pageIDTexture的格式完全一致
		Int2 pageID((removePage >> 20) & 0xFFF, (removePage >> 8) & 0xFFF);
		uint32_t gpuMip = (removePage >> 4) & 0xF;
		uint32_t log2IndiTexSize = (removePage >> 0) & 0xF;

		if (!readbackSets.contains(removePage))
			m_cbDataUpdateIndex.push_back(CBufferPhysPageUpdateIndex(-1, pageID, gpuMip));
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
