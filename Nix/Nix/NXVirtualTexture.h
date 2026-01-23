#pragma once
#include <array>
#include "NXInstance.h"
#include "BaseDefs/Math.h"
#include "NXConstantBuffer.h"
#include "NXVirtualTextureCommon.h"
#include "NXVTImageQuadTree.h"

struct NXVTSector
{
	NXVTSector() : id(0, 0), imageSize(0) {}
	NXVTSector(const Int2& id, int imageSize) : id(id), imageSize(imageSize) {}
	
	// 需要两种Key，位置id唯一key 和 全局唯一key
	uint64_t GetKey() { return id.x << 16 | id.y; }

	bool operator==(const NXVTSector& other) const noexcept {
		return id == other.id && imageSize == other.imageSize;
	}

	Int2 id;
	int imageSize;
};

template<>
struct std::hash<NXVTSector>
{
	size_t operator()(const NXVTSector& sector) const noexcept
	{
		return (static_cast<size_t>(sector.id.x) << 32) ^ (static_cast<size_t>(sector.id.y) << 16) ^ static_cast<size_t>(sector.imageSize);
	}
};

struct NXVTChangeSector
{
	NXVTSector oldData;
	int changedImageSize;
};

class NXVirtualTexture
{
	constexpr static size_t VTIMAGE_MAX_NODE_SIZE = 256; // 最大node使用的分辨率

	constexpr static size_t VTSECTOR_LOD_NUM = 7; // VTSector的lod等级

	constexpr static size_t SECTOR_SIZE = 64;
	constexpr static float	SECTOR_SIZEF = 64.0f;
	constexpr static float	SECTOR_SIZEF_INV = 1.0 / SECTOR_SIZEF;
	constexpr static size_t SECTOR_SIZE_LOG2 = 6;

public:
	NXVirtualTexture();
	~NXVirtualTexture();

	void Init(class NXCamera* pCam) { m_pCamera = pCam; }
	void Update();

	void UpdateCBData(const Vector2& rtSize)
	{
		m_cbDataVTReadback = Vector4(rtSize.x, rtSize.y, 0.0f, 0.0f);
		m_cbVTReadback.Update(m_cbDataVTReadback);
	}

	const NXConstantBuffer<Vector4>& GetCBufferVTReadback() const { return m_cbVTReadback; }

	void Release();

private:
	void UpdateNearestSectors();

	// 获取sector-相机最近距离的 平方
	float GetDist2OfSectorToCamera(const Vector2& camPos, const Int2& sectorPos);

	int GetVTImageSizeFromDist2(const float dist2);

private:
	NXCamera* m_pCamera;
	std::array<float, VTSECTOR_LOD_NUM> m_vtSectorLodDists;
	int m_vtSectorLodMaxDist;

	std::vector<NXVTSector> m_sectors;
	std::vector<NXVTSector> m_lastSectors;

	Vector4 m_cbDataVTReadback;
	NXConstantBuffer<Vector4> m_cbVTReadback;

	// 四叉树VirtImage
	NXVTImageQuadTree* m_pVirtImageQuadTree;

	// 记录一个sector到VirtImage位置的映射
	std::unordered_map<NXVTSector, Int2> m_sector2VirtImagePos;
};
