#include "NXVirtualTexture.h"
#include "NXCamera.h"

NXVirtualTexture::NXVirtualTexture(class NXCamera* pCam) :
	m_pCamera(pCam),
	m_vtSectorLodDists({ 32.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f }),
	m_vtSectorLodMaxDist(400)
{
	m_pVirtImageQuadTree = new NXVTImageQuadTree();
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

	for (auto& s : createSector)
	{
		Int2 virtImgPos = m_pVirtImageQuadTree->Alloc(s.imageSize);
		printf("CreateSector: %d %d\n", virtImgPos.x, virtImgPos.y);
		m_sector2VirtImagePos[s] = virtImgPos;
	}

	for (auto& s : removeSector)
	{
		if (m_sector2VirtImagePos.find(s) != m_sector2VirtImagePos.end())
		{
			Int2 virtImgPos = m_sector2VirtImagePos[s];
			m_pVirtImageQuadTree->Free(virtImgPos, s.imageSize);
			m_sector2VirtImagePos.erase(s);
		}
	}

	for (auto& s : changeSector)
	{
		Int2 newVirtImgPos = m_pVirtImageQuadTree->Alloc(s.changedImageSize);
		printf("changeSector: %d %d\n", newVirtImgPos.x, newVirtImgPos.y);
		NXVTSector newSector = s.oldData;
		newSector.imageSize = s.changedImageSize;
		m_sector2VirtImagePos[newSector] = newVirtImgPos;
		if (m_sector2VirtImagePos.find(s.oldData) != m_sector2VirtImagePos.end())
		{
			Int2 oldVirtImgPos = m_sector2VirtImagePos[s.oldData];
			m_pVirtImageQuadTree->Free(oldVirtImgPos, s.oldData.imageSize);
			m_sector2VirtImagePos.erase(s.oldData);
		}
	}
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
