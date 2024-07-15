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

// 2024.7.15 PSO������
// ����DX12��Ҫ�ֶ�������Դ����˿��ܻ����PSO In flighting������ʱ����Ҫ����PSO�����
// �����Ҫר����һ��PSO������������PSO���������ڣ���������DX11����������ɾ��
class NXPSOManager : public NXInstance<NXPSOManager>
{
public:
	NXPSOManager() : m_nFenceValue(0) {}
	~NXPSOManager() {}

	void Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue);

	// �ֳ����������
	// ���name�����ڣ�����PSO
	// ���name�Ѿ����ڣ����˴���PSO�����Ὣԭ����PSO���ΪWaitForRelease���ȴ�GPU����ʹ��ʱ�ͷţ�
	ID3D12PipelineState* Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const std::string& name);

	// ÿ֡����ʱ���ã������Ѿ����滻��PSO
	void FrameCleanup();

private:
	ID3D12CommandQueue* m_pCmdQueue;
	ID3D12Fence* m_pFence;

	std::unordered_map<std::string, NXPSOData> m_psoMap;
	std::list<NXPSOData> m_psoWaitForReleaseList;
	UINT64 m_nFenceValue;
};
