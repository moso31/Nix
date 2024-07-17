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

	// 清理已经被替换的PSO.
	// 由于GPU是异步的，因此需要等待GPU信号，才能释放PSO
	// 只要currentGPUFenceValue > PSO的frameValue，就说明该PSO没有GPU占用，可以释放了
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
