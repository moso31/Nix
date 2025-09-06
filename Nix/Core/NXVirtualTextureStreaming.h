#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include "NXTexture.h"

struct NXVTInfoTask
{
	// 哪个terrain
	Int2 terrainID; 
	// 记录具体是哪个sector的哪块pixel，注意仍是全局的，不是单个terrain内的相对位置
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

	// 地形块的工作目录
	std::filesystem::path m_terrainWorkingDir;

	// 异步相关
	std::mutex m_mutex;

	// 纹理加载任务队列
	std::vector<NXVTInfoTask> m_infoTasks;
	std::vector<NXVTTexTask> m_texTasks;
	std::vector<NXVTTexTask> m_pendingTextures;

	std::vector<NXVirtualTextureTaskFinishData> m_pendingList;
};
