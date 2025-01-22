#pragma once
#include <unordered_map>
#include <list>
#include "NXInstance.h"
#include "BaseDefs/DX12.h"

struct NXPSOData
{
	ComPtr<ID3D12PipelineState> data;
	UINT64 fenceValue;
};

// 2024.7.15 PSO������
// ����DX12��Ҫ�ֶ�������Դ����˿��ܻ����PSO In flighting������ʱ����Ҫ����PSO�����
// �����Ҫר����һ��PSO������������PSO���������ڣ���������DX11����������ɾ��
class NXPSOManager : public NXInstance<NXPSOManager>
{
public:
	NXPSOManager() {}
	virtual ~NXPSOManager() {}

	void Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue);

	// �ֳ����������
	// ���name�����ڣ�����PSO
	// ���name�Ѿ����ڣ����˴���PSO�����Ὣԭ����PSO���ΪWaitForRelease���ȴ�GPU����ʹ��ʱ�ͷţ�
	ID3D12PipelineState* Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const std::string& name);

	// ÿ֡����ʱ���ã������Ѿ����滻��PSO
	void FrameCleanup();

private:
	ID3D12CommandQueue* m_pCmdQueue;

	std::unordered_map<std::string, NXPSOData> m_psoMap;
	std::list<NXPSOData> m_psoWaitForReleaseList;
};
