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

	// 遍历所有任务，执行
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

	// 这一步从磁盘读对应的位置的纹理效果，目的是把splatmap、（将来还有heightmap）、decal都加载到内存，形成 一个data包
	// 然后VT烘焙管线再使用每个data包做自己的drawcall，画出真正的纹理来
	m_loadTasks.push_back(task);
}
