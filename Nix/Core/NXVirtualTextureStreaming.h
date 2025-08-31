#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/Math.h"

struct NXVirtualTextureTask
{
	// 记录具体是哪个sector的哪块pixel
	Int2 sectorXY;
	Int2 pxPos;
	Int2 pxSize;
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

	void AddTexLoadTask(const NXVirtualTextureTask& task);

private:
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
	ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_nFenceValue;

	// 地形块的工作目录
	std::filesystem::path m_terrainWorkingDir;

	std::mutex m_mutex;
	std::vector<NXVirtualTextureTask> m_loadTasks;
	std::vector<NXVirtualTextureTaskFinishData> m_pendingList;
};
