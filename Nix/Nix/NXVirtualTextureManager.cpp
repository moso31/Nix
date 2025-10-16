#include "NXVirtualTextureManager.h"
#include "NXTerrainCommon.h"
#include <unordered_set>
#include "NXVirtualTextureStreaming.h"

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

	std::unordered_map<Int2, int, Int2VTHasher> newSectorMap;
	for (auto& sector : m_sectorXZ)
	{
		newSectorMap[sector] = CalcVirtImageSize(sector);
	}

	// 分4种情况，
	// 上一帧有，这一帧没有：删掉
	// 上一帧有，这一帧有，大小不变：保留
	// 上一帧有，这一帧有，大小变：添加+延迟删除
	std::unordered_set<NXSectorInfo> removeImmediately, addImmediately, removeDelayed;
	for (auto& oldSectorInfo : m_sectorInfo)
	{
		if (!newSectorMap.contains(oldSectorInfo.position))
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

	// 上一帧没有，这一帧有：添加
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
	
	// 基于上述结果，更新sector信息
	// TODO：立即移除和延迟移除目前还没有像AVT描述的那样做区别。看看后续会怎么样。
	for (auto& sector : addImmediately)
	{
		m_atlas->InsertImage(sector);
		
		NXVTInfoTask vtTask;
		vtTask.terrainID = GetTerrainIDFromWorldPos(sector.position);
		vtTask.sectorXY = sector.position;
		vtTask.tileRelativePos = GetRelativeTerrainPosFromWorldPos(sector.position);
		vtTask.tileSize = g_terrainConfig.SectorSize;
		NXVTStreaming->AddTexLoadTask(vtTask);
	}
	for (auto& sector : removeImmediately)
		m_atlas->RemoveImage(sector);
	for (auto& sector : removeDelayed)
		m_atlas->RemoveImage(sector);

	m_sectorInfo.insert(m_sectorInfo.end(), addImmediately.begin(), addImmediately.end());
	std::erase_if(m_sectorInfo, [&removeImmediately](const NXSectorInfo& oldSectorInfo) { return removeImmediately.contains(oldSectorInfo); });
	std::erase_if(m_sectorInfo, [&removeDelayed](const NXSectorInfo& oldSectorInfo) { return removeDelayed.contains(oldSectorInfo); });
}

void NXVirtualTextureManager::UpdateCBData(const Vector2& rtSize)
{
	m_cbDataVTReadback.param0 = Vector4(rtSize.x, rtSize.y, 0.0f, 0.0f);
	m_cbVTReadback.Update(m_cbDataVTReadback);
}

int NXVirtualTextureManager::CalcVirtImageSize(const Int2& sector)
{
	Vector3 sectorPosWS((float)sector.x, 0.0f, (float)sector.y);
	Vector3 camPosWS = m_pCamera->GetTranslation();
	float distance = Vector3::Distance(sectorPosWS, camPosWS);
	float t = distance / 128.0f; // TODO: 128.0?...

	int lod = 0;
	if (t > 1) lod = (int)std::log2(t + 1);

	int resolution = (NXVT_VIRTUALIMAGE_MAXNODE_PIXEL / NXVT_VIRTUALIMAGE_MAXNODE) >> lod;
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

Int2 NXVirtualTextureManager::GetTerrainIDFromWorldPos(const Int2& worldPosXZ)
{
	// 偏移到正坐标
	Int2 PositivePos = worldPosXZ - g_terrainConfig.MinTerrainPos;

	int ix = static_cast<int>(std::floor(PositivePos.x / g_terrainConfig.TerrainSize));
	int iy = static_cast<int>(std::floor(PositivePos.y / g_terrainConfig.TerrainSize));
	return Int2(ix, iy);
}

Int2 NXVirtualTextureManager::GetRelativeTerrainPosFromWorldPos(const Int2& sectorPos)
{
	// 偏移到正坐标
	Int2 PositivePos = sectorPos - g_terrainConfig.MinTerrainPos;
	int rX = PositivePos.x % g_terrainConfig.TerrainSize;
	int rY = PositivePos.y % g_terrainConfig.TerrainSize;
	return Int2(rX, rY);
}
