#pragma once
#include "NXVirtualTextureCommon.h"
#include "NXCamera.h"
#include "NXInstance.h"

class NXVirtualTextureManager : public NXInstance<NXVirtualTextureManager>
{
public:
	const float NXVT_SECTORSIZE = 64.0f;
	const int NXVT_SECTORSIZE_LOG2 = 6;
	const int NXVT_VIRTUALIMAGE_MAXNODE = 256; // VirtualImage �����ڵ�������
	const int NXVT_PHYSICAL_TILE_SIZE = 256; // VirtualImage �������ض�Ӧ������Tile��С
	const int NXVT_VIRTUALIMAGE_MAXNODE_PIXEL = (NXVT_VIRTUALIMAGE_MAXNODE * NXVT_PHYSICAL_TILE_SIZE); // VirtualImage �����ڵ����������

	void Init();

	void SetCamera(NXCamera* pCamera) { m_pCamera = pCamera; }
	const std::vector<Int2>& GetSectorList() const { return m_sectorXZ; }

	// �������λ�ú;��룬���������б�
	void BuildSearchList(float distance);
	void Update();

	int CalcVirtImageSize(const Int2& sector);

	void Release();

private:
	NXCamera* m_pCamera = nullptr;

	std::vector<Int2> m_offsetXZ; // ���渽��һ�������ڵ�ƫ����������������������sector

	std::vector<Int2> m_sectorXZ;
	std::vector<NXSectorInfo> m_sectorInfo;

	NXVirtualTextureAtlas* m_atlas;
};
