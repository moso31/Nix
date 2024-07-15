#pragma once
#include <unordered_map>
#include <list>
#include "NXInstance.h"
#include "BaseDefs/DX12.h"

enum NXPSOState
{
	NXPSOState_Work,
	NXPSOState_WaitForRelease,
	NXPSOState_Released,
};

struct NXPSOData
{
	ComPtr<ID3D12PipelineState> data;
	NXPSOState state;
	UINT64 fenceValue;
};

// 2024.7.15 PSO管理器
// 由于DX12需要手动管理资源，因此可能会出现PSO In flighting，但此时又需要更换PSO的情况
// 因此需要专门做一个PSO管理器，管理PSO的生命周期，不能再像DX11那样随用随删了
class NXPSOManager : public NXInstance<NXPSOManager>
{
public:
	NXPSOManager() : m_nFenceValue(0) {}
	~NXPSOManager() {}

	void Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue);

	// 分成两种情况，
	// 如果name不存在，创建PSO
	// 如果name已经存在，除了创建PSO，还会将原来的PSO标记为WaitForRelease，等待GPU不再使用时释放；
	ID3D12PipelineState* Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const std::string& name);

	// 每帧结束时调用，清理已经被替换的PSO
	void FrameCleanup();

private:
	ID3D12CommandQueue* m_pCmdQueue;
	ID3D12Fence* m_pFence;

	std::unordered_map<std::string, NXPSOData> m_psoMap;
	std::list<NXPSOData> m_psoWaitForReleaseList;
	UINT64 m_nFenceValue;
};
