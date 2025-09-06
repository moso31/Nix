#include "NXVirtualTextureStreaming.h"
#include "NXGlobalDefinitions.h"
#include "NXAllocatorManager.h"
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

	// 如果已经有很多纹理在处理了，就先不推新的
	static constexpr size_t kMaxConcurrent = 5;
	if (m_texTasks.size() > kMaxConcurrent)
		return;

	int cnt = 0;
	auto it = m_infoTasks.begin();
	while (it != m_infoTasks.end() && m_texTasks.size() < kMaxConcurrent)
	{
		auto& task = *it;

		// 加载高度图
		std::string strTerrId = std::to_string(task.terrainID.x) + "_" + std::to_string(task.terrainID.y);
		std::string strRaw = std::format("tile_{:02}_{:02}.raw", task.terrainID.x, task.terrainID.y);
		std::filesystem::path texHeightMapPath = m_terrainWorkingDir / strTerrId / strRaw;
		std::string debugName = "VT_HeightMap_" + strTerrId + "_tile" + std::to_string(task.tileRelativePos.x) + "_" + std::to_string(task.tileRelativePos.y);

		// 加载SplatMap图
		std::filesystem::path texSplatMapPath = m_terrainWorkingDir / "splatmap_uncompress.dds";

		NXVTTexTask nextTask;
		nextTask.pHeightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(debugName, texHeightMapPath, task.tileRelativePos, task.tileSize + 1); 
		nextTask.pSplatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(debugName, texHeightMapPath, task.tileRelativePos, task.tileSize + 1);
		m_texTasks.push_back(nextTask);

		it = m_infoTasks.erase(it);
	}

	auto it2 = m_texTasks.begin();
	while (it2 != m_texTasks.end())
	{
		auto& task = *it2;
		if (task.pSplatMap.IsValid() && task.pSplatMap->IsLoadReady() && task.pHeightMap.IsValid() && task.pHeightMap->IsLoadReady())
		{
			NXVTTexTask pendingTask = task;
			m_pendingTextures.push_back(pendingTask);
			it2 = m_texTasks.erase(it2);
		}
		else
		{
			++it2;
		}
	}
}

void NXVirtualTextureStreaming::AddTexLoadTask(const NXVTInfoTask& task)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 这一步从磁盘读对应的位置的纹理效果，目的是把splatmap、（将来还有heightmap）、decal都加载到内存，形成 一个data包
	// 然后VT烘焙管线再使用每个data包做自己的drawcall，画出真正的纹理来
	m_infoTasks.push_back(task);
}
