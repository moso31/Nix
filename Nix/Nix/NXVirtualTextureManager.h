#pragma once
#include "NXCamera.h"
#include "NXInstance.h"

struct NXVTSector
{
	// VT Sector ������λ��
	// ������ 64x64 �Ĵ�С
	Int2 posXZ;
};

class NXVirtualTextureManager : public NXInstance<NXVirtualTextureManager>
{
public:
	const float NXVT_SECTORSIZE = 64.0f;
	const int NXVT_SECTORSIZE_LOG2 = 6;

	void SetCamera(NXCamera* pCamera) { m_pCamera = pCamera; }
	const std::vector<Int2>& GetSectorList() const { return m_sectorXZ; }

	// �������λ�ú;��룬���������б�
	void BuildSearchList(float distance);
	void Update();

private:
	NXCamera* m_pCamera = nullptr;

	std::vector<NXVTSector> m_vtSectors;
	std::vector<Int2> m_offsetXZ; // ���渽��һ�������ڵ�ƫ����������������������sector
	std::vector<Int2> m_sectorXZ;
};
