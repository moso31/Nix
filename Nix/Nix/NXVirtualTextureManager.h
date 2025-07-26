#pragma once
#include "NXCamera.h"
#include "NXInstance.h"

struct NXVTSector
{
	// VT Sector 的中心位置
	// 必须是 64x64 的大小
	Int2 posXZ;
};

class NXVirtualTextureManager : public NXInstance<NXVirtualTextureManager>
{
public:
	const float NXVT_SECTORSIZE = 64.0f;
	const int NXVT_SECTORSIZE_LOG2 = 6;

	void SetCamera(NXCamera* pCamera) { m_pCamera = pCamera; }
	const std::vector<Int2>& GetSectorList() const { return m_sectorXZ; }

	// 根据相机位置和距离，构建搜索列表
	void BuildSearchList(float distance);
	void Update();

private:
	NXCamera* m_pCamera = nullptr;

	std::vector<NXVTSector> m_vtSectors;
	std::vector<Int2> m_offsetXZ; // 缓存附近一定距离内的偏移量，用这个找相机附近的sector
	std::vector<Int2> m_sectorXZ;
};
