#include "NXVirtualTexture.h"
#include "NXResourceManager.h"
#include "NXCamera.h"
#include "NXTexture.h"
#include <unordered_set>

NXVirtualTexture::NXVirtualTexture(class NXCamera* pCam) :
	m_pCamera(pCam),
	m_vtSectorLodDists({ 32.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f }),
	m_vtSectorLodMaxDist(400),
	m_lruCache(LRU_CACHE_SIZE)
{
	m_pVirtImageQuadTree = new NXVTImageQuadTree();

	m_pSector2IndirectTexture = NXManager_Tex->CreateTexture2D("VirtualTexture_Sector2IndirectTexture", DXGI_FORMAT_R32_UINT, VT_SECTOR2INDIRECTTEXTURE_SIZE, VT_SECTOR2INDIRECTTEXTURE_SIZE, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pSector2IndirectTexture->SetViews(1, 0, 0, 1);
	m_pSector2IndirectTexture->SetSRV(0);
	m_pSector2IndirectTexture->SetUAV(0);

	m_pPhysicalPageAlbedo = NXManager_Tex->CreateTexture2DArray("VirtualTexture_PhysicalPage_Albedo", DXGI_FORMAT_R8G8B8A8_UNORM, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileNum, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pPhysicalPageNormal = NXManager_Tex->CreateTexture2DArray("VirtualTexture_PhysicalPage_Normal", DXGI_FORMAT_R8G8B8A8_UNORM, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileSize, g_virtualTextureConfig.PhysicalPageTileNum, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	int mip = 11;
	m_pIndirectTexture = NXManager_Tex->CreateTexture2D("VirtualTexture_IndirectTexture", DXGI_FORMAT_R32_UINT, g_virtualTextureConfig.IndirectTextureSize, g_virtualTextureConfig.IndirectTextureSize, mip, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pIndirectTexture->SetViews(1, 0, 0, mip);
	m_pIndirectTexture->SetSRV(1);
	for (int i = 0; i < mip; i++)
		m_pIndirectTexture->SetUAV(i, i);

	m_cbSector2IndirectTexture.Recreate(CB_SECTOR2INDIRECTTEXTURE_DATA_NUM);
	m_cbPhysPageBake.Recreate(CB_PHYSPAGEBAKEDATA_NUM);
	m_cbPhysPageUpdateIndex.Recreate(BAKE_PHYSICAL_PAGE_PER_FRAME);

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
	m_lastSectors.swap(m_sectors);
	m_sectors.clear();
	UpdateNearestSectors();
	BakePhysicalPages();
	UpdateIndirectTexture();
}

void NXVirtualTexture::UpdateCBData(const Vector2& rtSize)
{
	m_cbDataVTReadback = Vector4(rtSize.x, rtSize.y, 0.0f, 0.0f);
	m_cbVTReadback.Update(m_cbDataVTReadback);
}

void NXVirtualTexture::UpdateNearestSectors()
{
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

	// 准备更新 Sector2IndirectTexture
	m_cbDataSector2IndirectTexture.clear();
	for (auto& s : createSector)
	{
		Int2 virtImgPos = m_pVirtImageQuadTree->Alloc(s.imageSize, s.id);
		m_sector2VirtImagePos[s] = virtImgPos;
		m_cbDataSector2IndirectTexture.push_back({ s.id - g_terrainConfig.MinSectorID, virtImgPos, s.imageSize });
	}

	for (auto& s : removeSector)
	{
		if (m_sector2VirtImagePos.find(s) != m_sector2VirtImagePos.end())
		{
			Int2 virtImgPos = m_sector2VirtImagePos[s];
			m_pVirtImageQuadTree->Free(virtImgPos, s.imageSize);
			m_sector2VirtImagePos.erase(s);
			m_cbDataSector2IndirectTexture.push_back({ s.id - g_terrainConfig.MinSectorID, -1 });
		}
	}

	for (auto& s : changeSector)
	{
		Int2 newVirtImgPos = m_pVirtImageQuadTree->Alloc(s.changedImageSize, s.oldData.id);
		NXVTSector newSector = s.oldData;
		newSector.imageSize = s.changedImageSize;
		m_sector2VirtImagePos[newSector] = newVirtImgPos;
		m_cbDataSector2IndirectTexture.push_back({ s.oldData.id - g_terrainConfig.MinSectorID, newVirtImgPos, s.changedImageSize });

		if (m_sector2VirtImagePos.find(s.oldData) != m_sector2VirtImagePos.end())
		{
			Int2 oldVirtImgPos = m_sector2VirtImagePos[s.oldData];
			m_pVirtImageQuadTree->Free(oldVirtImgPos, s.oldData.imageSize);
			m_sector2VirtImagePos.erase(s.oldData);
			//m_cbDataSector2IndirectTexture.push_back({ s.oldData.id - g_terrainConfig.MinSectorID, -1 });
		}
	}

	if (m_cbDataSector2IndirectTexture.size() >= 256)
	{
		printf("WARNING\n");
	}

	// 准备更新 Sector2IndirectTexture
	m_cbSector2IndirectTexture.Update(m_cbDataSector2IndirectTexture);
	m_cbSector2IndirectTextureNum.Update((int)m_cbDataSector2IndirectTexture.size());
}

void NXVirtualTexture::BakePhysicalPages()
{
	if (m_vtReadbackData.IsNull())
		return;

	auto pVTReadbackData = m_vtReadbackData->Clone();
	if (pVTReadbackData.empty())
		return;

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

	int lruInsertNum = 0;
	m_cbDataPhysPageBake.clear();
	m_cbDataPhysPageUpdateIndex.clear();

	for (auto& data : readbackSets)
	{
		Int2 pageID((data >> 20) & 0xFFF, (data >> 8) & 0xFFF);
		uint32_t gpuMip = (data >> 4) & 0xF;
		Int2 pageIDMip0 = pageID << gpuMip;
		uint32_t log2IndiTexSize = (data >> 0) & 0xF;

		Int2 sectorID = m_pVirtImageQuadTree->GetSector(pageIDMip0, log2IndiTexSize);
		if (sectorID != Int2(INT_MIN))
		{
			NXVTLRUKey key;
			key.sector = sectorID;
			key.pageID = pageID;
			key.gpuMip = gpuMip;
			key.indiTexLog2Size = log2IndiTexSize;

			uint64_t keyHash = key.GetKey();
			if (m_lruCache.Find(keyHash))
			{
				m_lruCache.Touch(keyHash);
			}
			else
			{
				int cacheIdx = m_lruCache.Insert(keyHash);
				lruInsertNum++;

				m_cbDataPhysPageBake.push_back(key);
				m_cbDataPhysPageUpdateIndex.push_back(CBufferPhysPageUpdateIndex(cacheIdx, pageID, gpuMip));

				//printf("Sector: (%d, %d), PageID: (%d, %d), GPU Mip: %d, IndiTexLog2Size: %d\n", key.sector.x, key.sector.y, key.pageID.x, key.pageID.y, key.gpuMip, key.indiTexLog2Size);

				if (lruInsertNum >= BAKE_PHYSICAL_PAGE_PER_FRAME)
					break;
			}
		}
	}

	for (auto& idx : m_cbDataPhysPageUpdateIndex)
	{
		printf("%d ", idx.index);
	}
	if (!m_cbDataPhysPageUpdateIndex.empty()) printf("\n");

	m_cbPhysPageBake.Update(m_cbDataPhysPageBake);
	m_cbPhysPageUpdateIndex.Update(m_cbDataPhysPageUpdateIndex);
}

void NXVirtualTexture::UpdateIndirectTexture()
{

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
	return -1;
}
