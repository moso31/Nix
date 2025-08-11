#pragma once
#include "NXVirtualTextureCommon.h"
#include "NXCamera.h"
#include "NXInstance.h"
#include "NXConstantBuffer.h"

struct CBufferVTReadback
{
	Vector4 param0;
};

class NXVirtualTextureManager : public NXInstance<NXVirtualTextureManager>
{
public:
	const float NXVT_SECTORSIZE = 64.0f;
	const int NXVT_SECTORSIZE_LOG2 = 6;
	const int NXVT_VIRTUALIMAGE_MAXNODE = 256; // VirtualImage 单个节点最大面积
	const int NXVT_PHYSICAL_TILE_SIZE = 256; // VirtualImage 单个像素对应的物理Tile大小
	const int NXVT_VIRTUALIMAGE_MAXNODE_PIXEL = (NXVT_VIRTUALIMAGE_MAXNODE * NXVT_PHYSICAL_TILE_SIZE); // VirtualImage 单个节点最大像素数

	void Init();

	NXVTAtlasQuadTreeNode* GetAtlasRootNode() { return m_atlas->GetRootNode(); }
	const std::vector<NXVTAtlasQuadTreeNode*>& GetNodes() { return m_atlas->GetNodes(); }
	void GetImagePosAndSize(NXVTAtlasQuadTreeNode* pNode, Int2& oAtlasPos, int& oAtlasSize);

	void SetCamera(NXCamera* pCamera) { m_pCamera = pCamera; }
	const std::vector<Int2>& GetSectorList() const { return m_sectorXZ; }
	const std::vector<NXSectorInfo> GetSectorInfos() { return m_sectorInfo; }

	// 根据相机位置和距离，构建搜索列表
	void BuildSearchList(float distance);
	void Update();
	void UpdateCBData(const Vector2& rtSize);

	int CalcVirtImageSize(const Int2& sector);

	void Release();

	NXConstantBuffer<CBufferVTReadback>& GetCBufferVTReadback() { return m_cbVTReadback; }

private:
	CBufferVTReadback m_cbDataVTReadback;
	NXConstantBuffer<CBufferVTReadback>	m_cbVTReadback;

	NXCamera* m_pCamera = nullptr;

	std::vector<Int2> m_offsetXZ; // 缓存附近一定距离内的偏移量，用这个找相机附近的sector

	std::vector<Int2> m_sectorXZ;
	std::vector<NXSectorInfo> m_sectorInfo;

	NXVirtualTextureAtlas* m_atlas;
};
