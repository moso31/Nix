#pragma once
#include "BaseDefs/DX12.h"
#include "NXTerrainStreamingAsyncLoader.h"
#include <vector>

// 这个类只负责读取NXTerrainStreaming的完成task+输出
class NXTerrainStreamingBatcher
{
public:
    void Push(const NXTerrainStreamingLoadTextureResult& task);

	void Init();
	void Update();
	void Render();

private:
	ComPtr<ID3D12CommandQueue> m_pCmdQueue;
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCmdList;
	ComPtr<ID3D12RootSignature> m_pRootSig;
	ComPtr<ID3D12PipelineState> m_pCSO;

	std::vector<NXTerrainStreamingLoadTextureResult> m_completedTasks;
};
