#include "NXVirtualTextureStreaming.h"
#include "NXGlobalDefinitions.h"
#include "NXAllocatorManager.h"
#include "NXTexture.h"
#include "NXResourceManager.h"

NXVirtualTextureStreaming::NXVirtualTextureStreaming()
{
	m_terrainWorkingDir = "D:\\NixAssets\\terrainTest";
}

void NXVirtualTextureStreaming::Init()
{
	auto pDevice = NXGlobalDX::GetDevice();
	NX12Util::CreateCommands(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_pCmdAllocator.GetAddressOf(), m_pCmdList.GetAddressOf());
	m_pCmdList->SetName(L"VirtualTexture Streaming Command List");
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
}

void NXVirtualTextureStreaming::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// ������������ִ��
	for (int i = 0; i < m_loadTasks.size(); i++)
	{
		auto& task = m_loadTasks[i];
		task.sectorXY;
		//NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("");
	}
}

void NXVirtualTextureStreaming::AddTexLoadTask(const NXVirtualTextureTask& task)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// ��һ���Ӵ��̶���Ӧ��λ�õ�����Ч����Ŀ���ǰ�splatmap������������heightmap����decal�����ص��ڴ棬�γ� һ��data��
	// Ȼ��VT�決������ʹ��ÿ��data�����Լ���drawcall������������������
	m_loadTasks.push_back(task);
}
