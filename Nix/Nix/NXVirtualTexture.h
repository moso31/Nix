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
#include "NXVTDebugger.h"

// RenderGraph
#include "NXRGUtil.h"
class NXTerrainLODStreamer;
struct NXVTRenderGraphContext 
{
	class NXRenderGraph* pRG;
	NXTerrainLODStreamer* pTerrainLODStreamer;

	NXRGHandle hSector2VirtImg;
	NXRGHandle hSector2NodeIDTex;
	NXRGHandle hHeightMapAtlas;
	NXRGHandle hSplatMapAtlas;
	NXRGHandle hNormalMapAtlas;
	NXRGHandle hAlbedoMapArray;
	NXRGHandle hNormalMapArray;
	NXRGHandle hAlbedoPhysicalPage;
	NXRGHandle hNormalPhysicalPage;
	NXRGHandle hIndirectTexture;
};

enum class NXVTUpdateState
{
	None,
	Ready,
	WaitReadback,
	Reading,
	PhysicalPageBake,
	Finish
};

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

	Int2 id; // 世界sectorID
	int imageSize; // sector的大小，注意是log2size
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

struct CBufferRemoveSector
{
	Int2 imagePos;
	int imageSize;
	int maxRemoveMip; // 值=N时，表示只移除前N个mip（0~N-1）
};

struct CBufferMigrateSector
{
	Int2 fromImagePos;
	Int2 toImagePos;
	int fromImageSize;
	int toImageSize;
	int mipDelta; // 迁移前后两个sector的mip等级差
	int _0;
};

struct CBufferSector2VirtImg
{
	CBufferSector2VirtImg(const Int2& sector, const Int2& indiTexPos, int indiTexSize) : 
		sectorPos(sector),
		indiTexData(((indiTexPos.x & 0xFFF) << 20) | ((indiTexPos.y & 0xFFF) << 8) | (std::countr_zero((uint32_t)indiTexSize) & 0xFF)) // std::countr_zero = log2 of POTsize
	{
	}

	CBufferSector2VirtImg(const Int2& sector, int emptyData) : sectorPos(sector), indiTexData(emptyData) {}

	// 哪个像素
	Int2 sectorPos; 

	// 改成什么值
	int indiTexData; // x(12bit)y(12bit) = indi tex pos; z(8bit) = indi tex size
	int _0;
};  

struct NXVTReadbackPageData
{
	Int2 pageID;
	uint32_t gpuMip;
	uint32_t log2IndiTexSize;
};

class NXTexture2D;
class NXTexture2DArray;
class NXVirtualTexture
{
	constexpr static int VT_SECTOR2VIRTIMG_SIZE = 256; // Sector2VirtImg使用的分辨率
	constexpr static size_t VTIMAGE_MAX_NODE_SIZE = 256; // 最大node使用的分辨率
	constexpr static int CB_SECTOR2VIRTIMG_DATA_NUM = 256;

	constexpr static size_t VTSECTOR_LOD_NUM = 7; // VTSector的lod等级

	constexpr static size_t SECTOR_SIZE = 64;
	constexpr static float	SECTOR_SIZEF = 64.0f;
	constexpr static float	SECTOR_SIZEF_INV = 1.0 / SECTOR_SIZEF;
	constexpr static size_t SECTOR_SIZE_LOG2 = 6;

	constexpr static size_t BAKE_PHYSICAL_PAGE_PER_FRAME = 4; // 每帧最多烘焙的PhysicalPage数量
	constexpr static size_t UPDATE_INDIRECT_TEXTURE_PER_FRAME = 64; // 每帧最多更新的indirectTexture像素数

	constexpr static size_t INDIRECT_TEXTURE_SIZE = 2048;

public:
	NXVirtualTexture(class NXCamera* pCam, class NXTerrainLODStreamer* pTerrainLODStreamer);
	~NXVirtualTexture();

	void RegisterRenderPasses(const NXVTRenderGraphContext& ctx) { m_ctx = ctx; }
	void Update();

	void UpdateCBData(const Vector2& rtSize);
	const NXConstantBuffer<Vector4>& GetCBufferVTReadback() const { return m_cbVTReadback; }

	// sector2VirtualImage
	const Ntr<NXTexture2D>& GetSector2VirtImg() const { return m_pSector2VirtImg; }

	// physicalpage
	const Ntr<NXTexture2DArray>& GetPhysicalPageAlbedo() const { return m_pPhysicalPageAlbedo; }
	const Ntr<NXTexture2DArray>& GetPhysicalPageNormal() const { return m_pPhysicalPageNormal; }

	// updateindirecttexture
	const Ntr<NXTexture2D>& GetIndirectTexture() const { return m_pIndirectTexture; }

	// GUI 访问接口
	const std::vector<NXVTSector>& GetSectors() const { return m_sectors; }
	const NXVTImageQuadTree* GetQuadTree() const { return m_pVirtImageQuadTree; }
	const std::unordered_map<NXVTSector, Int2>& GetSector2VirtImagePos() const { return m_sector2VirtImagePos; }
	NXVTUpdateState GetUpdateState() const { return m_updateState; }
	void SetUpdateState(NXVTUpdateState s) { m_updateState = s; }

	// VT Readback 数据访问接口
	Ntr<NXReadbackData>& GetVTReadbackData() { return m_vtReadbackData; }
	const Int2& GetVTReadbackDataSize() const { return m_vtReadbackDataSize; }
	void SetVTReadbackDataSize(const Int2& val) { m_vtReadbackDataSize = val; }

	void UpdateStateAfterReadback() { m_bReadbackFinish = true; }

	void Release();

private:
	void RegisterClearSector2VirtImgPass();
	void RegisterClearIndirectTexturePass();
	void RegisterUpdateSector2VirtImgPass();
	void RegisterBakePhysicalPagePass();
	void RegisterUpdateIndirectTexturePass();
	void RegisterRemoveIndirectTextureSectorPass(const NXConstantBuffer<CBufferRemoveSector>& pCBRemoveSector, const CBufferRemoveSector& removeData);
	void RegisterMigrateIndirectTextureSectorPass(const NXConstantBuffer<CBufferMigrateSector>& pCBMigrateSector, const CBufferMigrateSector& migrateData);

private:
	void UpdateNearestSectors();
	void BakePhysicalPages();
	void DeduplicatePages(const std::vector<uint8_t>& pVTReadbackData);

	// 获取sector-相机最近距离的 平方
	float GetDist2OfSectorToCamera(const Vector2& camPos, const Int2& sectorPos);

	int GetVTImageSizeFromDist2(const float dist2);

private:
	NXCamera* m_pCamera;
	NXTerrainLODStreamer* m_pTerrainLODStreamer;

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
	Ntr<NXTexture2D> m_pSector2VirtImg;
	std::vector<CBufferSector2VirtImg> m_cbDataSector2VirtImg; 
	NXConstantBuffer<std::vector<CBufferSector2VirtImg>> m_cbSector2VirtImg;
	NXConstantBuffer<int> m_cbSector2VirtImgNum;

	// VT Readback 数据
	Ntr<NXReadbackData> m_vtReadbackData;
	Int2 m_vtReadbackDataSize;

	NXVTLRUCache m_lruCache;
	std::vector<uint32_t> m_physPageSlotSectorVersion;

	NXConstantBuffer<std::vector<NXVTLRUKey>> m_cbPhysPageBake;
	std::vector<NXVTLRUKey> m_cbDataPhysPageBake;
	NXConstantBuffer<std::vector<CBufferPhysPageUpdateIndex>> m_cbUpdateIndex;
	std::vector<CBufferPhysPageUpdateIndex> m_cbDataUpdateIndex;
	Ntr<NXTexture2DArray> m_pPhysicalPageAlbedo;
	Ntr<NXTexture2DArray> m_pPhysicalPageNormal;

	Ntr<NXTexture2D> m_pIndirectTexture;

	// UpdateNearestSector时配套的 页表(indirectTexture)同步
	std::vector<CBufferRemoveSector> m_cbDataRemoveSector;
	std::vector<CBufferRemoveSector> m_cbDataMigrateRemoveSector;
	std::vector<CBufferMigrateSector> m_cbDataMigrateSector;
	// 注意：每个struct单配CBuffer，而不是一整个CB<std::vector>特化！
	std::array<NXConstantBuffer<CBufferRemoveSector>, 1000> m_cbArrayRemoveSector; 
	std::array<NXConstantBuffer<CBufferRemoveSector>, 1000> m_cbArrayMigrateRemoveSector;
	std::array<NXConstantBuffer<CBufferMigrateSector>, 1000> m_cbArrayMigrateSector;

	bool m_bNeedClearSector2VirtImg = true; // 首帧清除标记
	bool m_bNeedClearIndirectTexture = true; // 首帧清除标记

	// 状态机，保证数据跨帧统一
	NXVTUpdateState m_updateState = NXVTUpdateState::None;
	bool m_bReadbackFinish = false;

	// 去重相关
	std::unordered_set<uint32_t> m_readbackSets;
	std::vector<NXVTReadbackPageData> m_duplicatedReadbackData;
	uint32_t m_bakeIndex = 0;

	// RenderGraph passes
	NXVTRenderGraphContext m_ctx;
};
