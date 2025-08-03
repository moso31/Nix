#include "NXVirtualTextureManager.h"
#include <unordered_set>

void NXVirtualTextureManager::Init()
{
	m_atlas = new NXVirtualTextureAtlas();
}

void NXVirtualTextureManager::BuildSearchList(float distance)
{
	m_offsetXZ.clear();
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
	// ��ȡ���λ��WS��������
	Vector3 posWS = m_pCamera->GetTranslation();
	posWS.x = std::floor(posWS.x / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
	posWS.z = std::floor(posWS.z / NXVT_SECTORSIZE) * NXVT_SECTORSIZE;
	Int2 posXZ = posWS.GetXZ() + Vector2(NXVT_SECTORSIZE);
	
	for (int i = 0; i < m_offsetXZ.size(); i++)
	{
		auto& ofsXZ = m_offsetXZ[i];
		m_sectorXZ[i] = posXZ + ofsXZ;
	}

	std::unordered_map<Int2, int, Int2VTHasher> newSectorMap;
	for (auto& sector : m_sectorXZ)
	{
		newSectorMap[sector] = CalcVirtImageSize(sector);
	}

	// ��4�������
	// ��һ֡�У���һ֡û�У�ɾ��
	// ��һ֡�У���һ֡�У���С���䣺����
	// ��һ֡�У���һ֡�У���С�䣺���+�ӳ�ɾ��
	std::unordered_set<NXSectorInfo> removeImmediately, addImmediately, removeDelayed;
	for (auto& oldSectorInfo : m_sectorInfo)
	{
		if (newSectorMap.contains(oldSectorInfo.position))
		{
			removeImmediately.insert(oldSectorInfo);
		}
		else
		{
			auto& newSectorSize = newSectorMap[oldSectorInfo.position];
			if (oldSectorInfo.size != newSectorSize)
			{
				removeDelayed.insert(oldSectorInfo);

				NXSectorInfo newInfo;
				newInfo.position = oldSectorInfo.position;
				newInfo.size = newSectorSize;
				addImmediately.insert(newInfo);
			}
		}
	}

	// ��һ֡û�У���һ֡�У����
	for (auto& [newSectorPos, newSectorSize] : newSectorMap)
	{
		if (std::find_if(m_sectorInfo.begin(), m_sectorInfo.end(), [newSectorPos](const NXSectorInfo& oldSector) { return newSectorPos == oldSector.position; }) == m_sectorInfo.end())
		{
			NXSectorInfo newInfo;
			newInfo.position = newSectorPos;
			newInfo.size = newSectorSize;
			addImmediately.insert(newInfo);
		}
	}
	
	// �����������������sector��Ϣ
	// TODO�������Ƴ����ӳ��Ƴ�Ŀǰ��û����AVT���������������𡣿�����������ô����
	for (auto& sector : addImmediately)
		m_atlas->InsertImage(sector);
	for (auto& sector : removeImmediately)
		m_atlas->RemoveImage(sector);
	for (auto& sector : removeDelayed)
		m_atlas->RemoveImage(sector);

	m_sectorInfo.insert(m_sectorInfo.end(), addImmediately.begin(), addImmediately.end());
	std::erase_if(m_sectorInfo, [&removeImmediately](const NXSectorInfo& oldSectorInfo) { return removeImmediately.contains(oldSectorInfo); });
	std::erase_if(m_sectorInfo, [&removeDelayed](const NXSectorInfo& oldSectorInfo) { return removeDelayed.contains(oldSectorInfo); });
}

int NXVirtualTextureManager::CalcVirtImageSize(const Int2& sector)
{
	Vector3 sectorPosWS((float)sector.x, 0.0f, (float)sector.y);
	Vector3 camPosWS = m_pCamera->GetTranslation();
	float distance = Vector3::Distance(sectorPosWS, camPosWS);
	float t = distance / 5.0f; // TODO: 5.0?...

	int lod = 0;
	if (t > 1) lod = (int)std::log2(t + 1);

	int resolution = NXVT_VIRTUALIMAGE_MAXNODE_PIXEL >> lod;
	return resolution;
}

void NXVirtualTextureManager::Release()
{
	if (m_atlas)
	{
		m_atlas->Release();
		m_atlas = nullptr;
	}
}
