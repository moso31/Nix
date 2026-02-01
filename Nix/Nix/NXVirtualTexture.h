#pragma once
#include <array>
#include <bit>
#include "NXInstance.h"
#include "BaseDefs/Math.h"
#include "NXConstantBuffer.h"
#include "NXVirtualTextureCommon.h"
#include "NXReadbackData.h"
#include "NXVTImageQuadTree.h"
#include "NXTerrainCommon.h"
#include "NXVTLRUCache.h"

struct NXVTSector
{
	NXVTSector() : id(0, 0), imageSize(0) {}
	NXVTSector(const Int2& id, int imageSize) : id(id), imageSize(imageSize) {}

	bool operator==(const NXVTSector& other) const noexcept {
		return id == other.id && imageSize == other.imageSize;
	}
	
	// 归并比较的小于
	bool Less(const NXVTSector& other)
	{
		if (id.x != other.id.x) return id.x < other.id.x;
		return id.y < other.id.y;
	}

	Int2 id;
	int imageSize;
};

struct CBufferSector2IndirectTexture
{
	CBufferSector2IndirectTexture(const Int2& sector, const Int2& indiTexPos, int indiTexSize) : 
		sectorPos(sector),
		indiTexData(((indiTexPos.x & 0xFFF) << 20) | ((indiTexPos.y & 0xFFF) << 8) | (std::countr_zero((uint32_t)indiTexSize) & 0xFF)) // std::countr_zero = log2 of POTsize
	{
	}

	CBufferSector2IndirectTexture(const Int2& sector, int emptyData) : sectorPos(sector), indiTexData(emptyData) {}

	// 哪个像素
	Int2 sectorPos; 

	// 改成什么值
	int indiTexData; // x(12bit)y(12bit) = indi tex pos; z(8bit) = indi tex size
	int _0;
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

class NXTexture2D;
class NXTexture2DArray;
class NXVirtualTexture
{
	constexpr static int VT_SECTOR2INDIRECTTEXTURE_SIZE = 256; // Sector2IndirectTexture使用的分辨率
	constexpr static size_t VTIMAGE_MAX_NODE_SIZE = 256; // 最大node使用的分辨率
	constexpr static int CB_SECTOR2INDIRECTTEXTURE_DATA_NUM = 256;
	constexpr static int CB_PHYSPAGEBAKEDATA_NUM = 4;

	constexpr static size_t VTSECTOR_LOD_NUM = 7; // VTSector的lod等级

	constexpr static size_t SECTOR_SIZE = 64;
	constexpr static float	SECTOR_SIZEF = 64.0f;
	constexpr static float	SECTOR_SIZEF_INV = 1.0 / SECTOR_SIZEF;
	constexpr static size_t SECTOR_SIZE_LOG2 = 6;

	constexpr static size_t LRU_CACHE_SIZE = 1024;
	constexpr static size_t BAKE_PHYSICAL_PAGE_PER_FRAME = 8; // 每帧最多烘焙的PhysicalPage数量

	constexpr static size_t INDIRECT_TEXTURE_SIZE = 2048;

public:
	NXVirtualTexture(class NXCamera* pCam);
	~NXVirtualTexture();

	void Update();

	void UpdateCBData(const Vector2& rtSize);
	const NXConstantBuffer<Vector4>& GetCBufferVTReadback() const { return m_cbVTReadback; }

	// sector2indirecttexture
	const Ntr<NXTexture2D>& GetSector2IndirectTexture() const { return m_pSector2IndirectTexture; }
	const NXConstantBuffer<std::vector<CBufferSector2IndirectTexture>>& GetCBufferSector2IndirectTexture() const { return m_cbSector2IndirectTexture; }
	const NXConstantBuffer<int>& GetCBufferSector2IndirectTextureNum() const { return m_cbSector2IndirectTextureNum; }
	const size_t GetCBufferSector2IndirectTextureDataNum() const { return m_cbDataSector2IndirectTexture.size(); }
	bool NeedClearSector2IndirectTexture() const { return m_bNeedClearSector2IndirectTexture; }
	void MarkSector2IndirectTextureCleared() { m_bNeedClearSector2IndirectTexture = false; }

	// physicalpage
	const Ntr<NXTexture2DArray>& GetPhysicalPageAlbedo() const { return m_pPhysicalPageAlbedo; }
	const Ntr<NXTexture2DArray>& GetPhysicalPageNormal() const { return m_pPhysicalPageNormal; }
	const NXConstantBuffer<std::vector<NXVTLRUKey>>& GetCBPhysPageBakeData() const { return m_cbPhysPageBake; }
	const size_t GetCBPhysPageBakeDataNum() const { return m_cbDataPhysPageBake.size(); }
	const NXConstantBuffer<std::vector<CBufferPhysPageUpdateIndex>>& GetCBPhysPageUpdateIndex() const { return m_cbPhysPageUpdateIndex; }

	// updateindirecttexture
	const Ntr<NXTexture2D>& GetIndirectTexture() const { return m_pIndirectTexture; }

	// GUI 访问接口
	const std::vector<NXVTSector>& GetSectors() const { return m_sectors; }
	const NXVTImageQuadTree* GetQuadTree() const { return m_pVirtImageQuadTree; }
	const std::unordered_map<NXVTSector, Int2>& GetSector2VirtImagePos() const { return m_sector2VirtImagePos; }

	// VT Readback 数据访问接口
	Ntr<NXReadbackData>& GetVTReadbackData() { return m_vtReadbackData; }
	const Int2& GetVTReadbackDataSize() const { return m_vtReadbackDataSize; }
	void SetVTReadbackDataSize(const Int2& val) { m_vtReadbackDataSize = val; }

	void Release();

private:
	void UpdateNearestSectors();
	void BakePhysicalPages();
	void UpdateIndirectTexture();

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

	// R32_UINT，24bit = 角坐标XY，8bit = size
	Ntr<NXTexture2D> m_pSector2IndirectTexture;

	bool m_bNeedClearSector2IndirectTexture = true; // 首帧清除标记
	std::vector<CBufferSector2IndirectTexture> m_cbDataSector2IndirectTexture; 
	NXConstantBuffer<std::vector<CBufferSector2IndirectTexture>> m_cbSector2IndirectTexture;
	NXConstantBuffer<int> m_cbSector2IndirectTextureNum;

	// VT Readback 数据
	Ntr<NXReadbackData> m_vtReadbackData;
	Int2 m_vtReadbackDataSize;

	NXVTLRUCache m_lruCache;
	NXConstantBuffer<std::vector<NXVTLRUKey>> m_cbPhysPageBake;
	std::vector<NXVTLRUKey> m_cbDataPhysPageBake;
	NXConstantBuffer<std::vector<CBufferPhysPageUpdateIndex>> m_cbPhysPageUpdateIndex;
	std::vector<CBufferPhysPageUpdateIndex> m_cbDataPhysPageUpdateIndex;
	Ntr<NXTexture2DArray> m_pPhysicalPageAlbedo;
	Ntr<NXTexture2DArray> m_pPhysicalPageNormal;

	Ntr<NXTexture2D> m_pIndirectTexture;
};
