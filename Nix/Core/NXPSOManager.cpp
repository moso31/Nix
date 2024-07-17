#include "NXPSOManager.h"
#include "NXGlobalDefinitions.h"
#include "NXConverter.h"

void NXPSOManager::Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue)
{
	m_pCmdQueue = pCmdQueue;
	m_nFenceValue = 0;

	m_pFence = NX12Util::CreateFence(pDevice, L"Create fence FAILED in NXPSOManager.");
}

ID3D12PipelineState* NXPSOManager::Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const std::string& name)
{
	auto it = m_psoMap.find(name);
	if (it != m_psoMap.end())
	{
		NXPSOData& oldPSO = it->second;
		oldPSO.state = NXPSOState_WaitForRelease;
		oldPSO.fenceValue = m_nFenceValue;
		m_psoWaitForReleaseList.push_back(std::move(oldPSO));
	}

	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_psoMap[name].data));
	m_psoMap[name].state = NXPSOState_Work;
	m_psoMap[name].data->SetName(NXConvert::s2ws(name).c_str());

	return m_psoMap[name].data.Get();
}

void NXPSOManager::FrameCleanup()
{
	m_pCmdQueue->Signal(m_pFence, m_nFenceValue++);

	// �����Ѿ����滻��PSO.
	// ����GPU���첽�ģ������Ҫ�ȴ�GPU�źţ������ͷ�PSO
	// ֻҪcurrentGPUFenceValue > PSO��frameValue����˵����PSOû��GPUռ�ã������ͷ���
	UINT64 currentGPUFenceValue = m_pFence->GetCompletedValue();
	std::erase_if(m_psoWaitForReleaseList, [currentGPUFenceValue](NXPSOData& psoData)
		{
			if (psoData.fenceValue <= currentGPUFenceValue)
			{
				psoData.data.Reset();
				return true;
			}
			return false;
		});
}
