#include "NXVirtualTextureManager.h"

void NXVirtualTextureManager::BuildSearchList(float distance)
{
	for (float x = -distance; x <= distance; x += NXVT_SECTORSIZE)
	{
		for (float z = -distance; z <= distance; z += NXVT_SECTORSIZE)
		{
			Vector2 sectorCenterPosXZ;
			sectorCenterPosXZ.x = std::floor(x / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
			sectorCenterPosXZ.y = std::floor(z / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
			sectorCenterPosXZ += Vector2(NXVT_SECTORSIZE);

			if (sectorCenterPosXZ.LengthSquared() <= distance * distance)
			{
				m_offsetXZ.push_back(sectorCenterPosXZ);
			}
		}
	}

	m_sectorXZ.resize(m_offsetXZ.size());
}

void NXVirtualTextureManager::Update()
{
	// 获取相机位置WS（整数）
	Vector3 posWS = m_pCamera->GetTranslation();
	posWS.x = std::floor(posWS.x / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
	posWS.z = std::floor(posWS.z / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
	Int2 posXZ = posWS.GetXZ() + Vector2(NXVT_SECTORSIZE);
	
	for (int i = 0; i < m_offsetXZ.size(); i++)
	{
		auto& ofsXZ = m_offsetXZ[i];
		m_sectorXZ[i].position = posXZ + ofsXZ;
	}

	std::vector<NXSectorInfo> delayedRemoveSectors;
	for (auto& sectorXZ : m_sectorXZ)
	{
		int virtImgSize = CalcVirtImageSize(sectorXZ.position);
		if (virtImgSize != sectorXZ.resolution)
		{
			delayedRemoveSectors.push_back(sectorXZ);
		}
	}
}

int NXVirtualTextureManager::CalcVirtImageSize(const Int2& sector)
{
	Vector3 sectorPosWS(sector.x, 0.0, sector.y);
	Vector3 camPosWS = m_pCamera->GetTranslation();
	float distance = Vector3::Distance(sectorPosWS, camPosWS);
	float t = distance / 5.0; // TODO: 5.0?...

	int lod = 0;
	if (t > 1) lod = std::log2(t + 1);

	int resolution = NXVT_VIRTUALIMAGE_MAXNODE_PIXEL >> lod;
	return resolution;
}
