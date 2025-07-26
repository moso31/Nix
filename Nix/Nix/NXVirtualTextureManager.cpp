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
		m_sectorXZ[i] = posXZ + ofsXZ;
	}
}
