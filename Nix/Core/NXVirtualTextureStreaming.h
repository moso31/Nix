#pragma once
#include "BaseDefs/DX12.h"
#include <memory>
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include "NXTexture.h"
#include "NXConstantBuffer.h"

struct NXVTFenceSync
{
	NXVTFenceSync();

	void ReadBegin(ID3D12CommandQueue* m_pCmdQueue);
	void ReadEnd(ID3D12CommandQueue* m_pCmdQueue);
	void WriteBegin(ID3D12CommandQueue* m_pCmdQueue);
	void WriteEnd(ID3D12CommandQueue* m_pCmdQueue);

	ComPtr<ID3D12Fence> pFenceRead;
	uint64_t fenceValueRead = 0;

	ComPtr<ID3D12Fence> pFenceWrite;
	uint64_t fenceValueWrite = 0;
};

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
	Int2 TileWorldPos;
	Int2 TileWorldSize;
};

struct NXVTTexTaskWithFence
{
	NXVTTexTask task;
	uint64_t fenceValue = 0;
};

struct CBufferVTConfig
{
	Int2 TileSize;
	int BakeTileNum;
	int _0;
};

struct CBufferVTBatch
{
	Int2 VTPageOffset;
	Int2 TileWorldPos;
	Int2 TileWorldSize;
	Int2 _0;
};

class NXVirtualTextureStreaming
{
public:
	NXVirtualTextureStreaming();
	~NXVirtualTextureStreaming();

	void AwakeOnce();

	void Init();
	void Update();
	void ProcessTasks();
	void ProcessVTBatch();

	void AddTexLoadTask(const NXVTInfoTask& task);

	NXVTFenceSync& GetFenceSync() { return m_fenceSync; }

private:
	// VT 单独使用一个 shader-visible descriptor allocator
	std::unique_ptr<DescriptorAllocator<true>> 	m_pShVisDescHeap;

	// DX12 
	ComPtr<ID3D12CommandQueue> m_pCmdQueue;
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
	ComPtr<ID3D12RootSignature> m_pRootSig;
	ComPtr<ID3D12PipelineState> m_pCSO;
	NXVTFenceSync m_fenceSync;

	// Tex 
	Ntr<NXTexture2DArray> m_pBaseColor2DArray;
	Ntr<NXTexture2D> m_pVTPhysicalPage0;
	Ntr<NXTexture2D> m_pVTPhysicalPage1;

	// CBuffer
	CBufferVTConfig m_cbVTConfigData;
	NXConstantBuffer<CBufferVTConfig> m_cbVTConfig;
	std::vector<CBufferVTBatch> m_cbVTBatchData;
	NXConstantBuffer<std::vector<CBufferVTBatch>> m_cbVTBatch;

	// 地形块的工作目录
	std::filesystem::path m_terrainWorkingDir;

	// 纹理加载任务队列
	std::vector<NXVTInfoTask> m_infoTasks;
	std::vector<NXVTTexTask> m_texTasks;
	std::vector<NXVTTexTask> m_pendingTextures;
	std::vector<NXVTTexTask> m_processingTextures;
	std::vector<NXVTTexTaskWithFence> m_waitGPUFinishTextures;

	ComPtr<ID3D12Fence> m_pFenceSubmit;
	uint64_t m_fenceValueSubmit = 0;
	HANDLE m_fenceEvent;

	// 异步相关
	std::mutex m_mutex;
	std::condition_variable m_cv;
	uint64_t m_lastFrame = 0;
};
