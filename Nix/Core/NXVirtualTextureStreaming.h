#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include "NXTexture.h"

struct NXVTInfoTask
{
	// �ĸ�terrain
	Int2 terrainID; 
	// ��¼�������ĸ�sector���Ŀ�pixel��ע������ȫ�ֵģ����ǵ���terrain�ڵ����λ��
	Int2 sectorXY;
	Int2 tileRelativePos;
	Int2 tileSize;
};

struct NXVTTexTask
{
	Ntr<NXTexture2D> pHeightMap;
	Ntr<NXTexture2D> pSplatMap;
};

struct NXVirtualTextureTaskFinishData
{

};

class NXVirtualTextureStreaming
{
public:
	NXVirtualTextureStreaming();
	~NXVirtualTextureStreaming() {}

	void Init();
	void Update();

	void AddTexLoadTask(const NXVTInfoTask& task);

private:
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
	ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_nFenceValue;

	// ���ο�Ĺ���Ŀ¼
	std::filesystem::path m_terrainWorkingDir;

	// �첽���
	std::mutex m_mutex;

	// ��������������
	std::vector<NXVTInfoTask> m_infoTasks;
	std::vector<NXVTTexTask> m_texTasks;
	std::vector<NXVTTexTask> m_pendingTextures;

	std::vector<NXVirtualTextureTaskFinishData> m_pendingList;
};
