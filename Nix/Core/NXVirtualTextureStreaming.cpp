#include "NXVirtualTextureStreaming.h"
#include "NXGlobalDefinitions.h"
#include "NXAllocatorManager.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXSamplerManager.h"
#include "NXVirtualTextureConfig.h"
#include "NXTerrainCommon.h"

NXVTFenceSync::NXVTFenceSync()
{
	pFenceRead = NX12Util::CreateFence(NXGlobalDX::GetDevice(), L"Create Fence Failed at NXVTFenceSync_read");
	pFenceRead->SetName(L"VT_Fence_Read");
	pFenceWrite = NX12Util::CreateFence(NXGlobalDX::GetDevice(), L"Create Fence Failed at NXVTFenceSync_write");
	pFenceWrite->SetName(L"VT_Fence_Write");
}

void NXVTFenceSync::ReadBegin(ID3D12CommandQueue* pCmdQueue) 
{
	pCmdQueue->Wait(pFenceWrite.Get(), fenceValueWrite);
}

void NXVTFenceSync::ReadEnd(ID3D12CommandQueue* pCmdQueue)
{
	fenceValueRead++;
	pCmdQueue->Signal(pFenceRead.Get(), fenceValueRead);
}

void NXVTFenceSync::WriteBegin(ID3D12CommandQueue* pCmdQueue)
{
	pCmdQueue->Wait(pFenceRead.Get(), fenceValueRead);
}

void NXVTFenceSync::WriteEnd(ID3D12CommandQueue* pCmdQueue)
{
	fenceValueWrite++;
	pCmdQueue->Signal(pFenceWrite.Get(), fenceValueWrite);
}

NXVirtualTextureStreaming::NXVirtualTextureStreaming()
{
	HANDLE m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	m_pFenceSubmit = NX12Util::CreateFence(NXGlobalDX::GetDevice(), L"Create Fence Failed at NXVTFenceSync_submit");
	m_pFenceSubmit->SetName(L"VT_Fence_Submit");

	m_terrainWorkingDir = "D:\\NixAssets\\terrainTest";
}

NXVirtualTextureStreaming::~NXVirtualTextureStreaming()
{
	CloseHandle(m_fenceEvent);
	m_fenceEvent = nullptr;
}

void NXVirtualTextureStreaming::AwakeOnce()
{
	m_cv.notify_one();
}

void NXVirtualTextureStreaming::Init()
{
	std::filesystem::path baseColor2DArrayPath = m_terrainWorkingDir / "basecolor_array.dds";
	m_pBaseColor2DArray = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DArray("VT_BaseColorArray", baseColor2DArrayPath.wstring());
	m_pBaseColor2DArray->WaitLoadingTexturesFinish();

	m_pVTPhysicalPage0 = NXResourceManager::GetInstance()->GetTextureManager()->CreateUAVTexture("VT_PhysicalPage_SplatMap", DXGI_FORMAT_R16_FLOAT, 8192, 8192,  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pVTPhysicalPage0->SetViews(1, 0, 0, 1);
	m_pVTPhysicalPage0->SetSRV(0);
	m_pVTPhysicalPage0->SetUAV(0);

	m_pVTPhysicalPage1 = NXResourceManager::GetInstance()->GetTextureManager()->CreateUAVTexture("VT_PhysicalPage_HeightMap", DXGI_FORMAT_R16_FLOAT, 8192, 8192, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
	m_pVTPhysicalPage1->SetViews(1, 0, 0, 1);
	m_pVTPhysicalPage1->SetSRV(0);
	m_pVTPhysicalPage1->SetUAV(0);

	m_cbVTBatchData.resize(g_VTConfig.MaxProcessingBatchesAtOnce);
	m_cbVTBatch.Recreate(g_VTConfig.MaxProcessingBatchesAtOnce);
	m_cbVTBatch.Set(m_cbVTBatchData);
	m_cbVTConfigData.TileSize = g_VTConfig.PhysicalPageTileSize;
	m_cbVTConfig.Set(m_cbVTConfigData);

	auto pDevice = NXGlobalDX::GetDevice();
	NX12Util::CreateCommands(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_pCmdQueue.GetAddressOf(), m_pCmdAllocator.GetAddressOf(), m_pCmdList.GetAddressOf());
	m_pCmdList->SetName(L"VirtualTexture Streaming Command List");

	m_pShVisDescHeap = std::make_unique<DescriptorAllocator<true>>(pDevice, 1000000, 10);

	// VT 管线专用 描述符规定：
	// Space0: 
	//		t0 = BaseColor2DArray
	//		u0, u1 = VTPhysicalPage(splatMap, HeightMap)
	// Space1:
	//		t0, t2, t4, ... = HeightMap2D
	//		t1, t3, t5, ... = SplatMap2D
	// Space2:
	//		t0, t1, t2, ... = DecalMap（预留，暂无实例）
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0), // space0
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0),
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_VTConfig.MaxRequestTerrainViewsPerUpdate, 0, 1), // space1
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_VTConfig.MaxRequestDecalViewsPerUpdate, 0, 2) // space2
	};

	std::vector<D3D12_ROOT_PARAMETER> rp = {
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL), // b0, space0
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL), // b1, space0
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> pSamplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL, D3D12_FILTER_MIN_MAG_MIP_POINT) // s0, ssPointWrap
	};

	m_pRootSig = NX12Util::CreateRootSignature(pDevice, rp, pSamplers);

	// CSO
	ComPtr<IDxcBlob> pCSBlob;
	NXShaderComplier::GetInstance()->CompileCS("Shader\\VTBatchBaker.fx", L"CS", pCSBlob.GetAddressOf());

	D3D12_COMPUTE_PIPELINE_STATE_DESC csoDesc = {};
	csoDesc.CS = { pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize() };
	csoDesc.pRootSignature = m_pRootSig.Get();
	pDevice->CreateComputePipelineState(&csoDesc, IID_PPV_ARGS(&m_pCSO));
}

void NXVirtualTextureStreaming::Update()
{
	uint64_t thisFrame;

	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [&]() { 
		return NXGlobalApp::s_frameIndex > m_lastFrame; 
	});

	thisFrame = NXGlobalApp::s_frameIndex;

	ProcessTasks();
	ProcessVTBatch();

	m_lastFrame = thisFrame;
}

void NXVirtualTextureStreaming::ProcessTasks()
{
	// 如果已经有很多纹理在处理了，就先不推新的
	if (m_texTasks.size() > g_VTConfig.MaxLoadTilesFromDiskAtOnce)
		return;

	// 加载高度图、SplatMap、贴花等
	auto it = m_infoTasks.begin();
	while (it != m_infoTasks.end() && m_texTasks.size() < g_VTConfig.MaxLoadTilesFromDiskAtOnce)
	{
		auto& task = *it;

		// 加载高度图
		std::string strTerrId = std::to_string(task.terrainID.x) + "_" + std::to_string(task.terrainID.y);
		std::string strRaw = std::format("tile_{:02}_{:02}.raw", task.terrainID.x, task.terrainID.y);
		std::filesystem::path texHeightMapPath = m_terrainWorkingDir / strTerrId / strRaw;
		std::string strHeightMapName = "VT_HeightMap_" + strTerrId + "_tile" + std::to_string(task.tileRelativePos.x) + "_" + std::to_string(task.tileRelativePos.y);

		// 加载SplatMap图
		std::filesystem::path texSplatMapPath = m_terrainWorkingDir / "splatmap_uncompress.dds";
		std::string strSplatMapName = "VT_SplatMap_" + strTerrId + "_tile" + std::to_string(task.tileRelativePos.x) + "_" + std::to_string(task.tileRelativePos.y);

		NXVTTexTask nextTask;
		nextTask.pHeightMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strHeightMapName, texHeightMapPath, task.tileRelativePos, task.tileSize + 1);
		nextTask.pSplatMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2DSubRegion(strSplatMapName, texSplatMapPath, task.tileRelativePos, task.tileSize + 1);
		nextTask.TileWorldPos = task.terrainID * g_terrainConfig.TerrainSize + task.tileRelativePos + g_terrainConfig.MinTerrainPos;
		nextTask.TileWorldSize = task.tileSize;
		m_texTasks.push_back(nextTask);

		it = m_infoTasks.erase(it);
	}

	// 如果纹理都加载好了，放到待处理队列
	// 纹理内置promise，加载好了会自动变成ready
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

	// 处理从磁盘加载好的tex。每帧数量是有上限的
	while (!m_pendingTextures.empty() && m_processingTextures.size() < g_VTConfig.MaxProcessingBatchesAtOnce)
	{
		m_processingTextures.push_back(m_pendingTextures.front()); // 这里的tex将会用于VT批渲染
		m_pendingTextures.erase(m_pendingTextures.begin());
	}
}

void NXVirtualTextureStreaming::ProcessVTBatch()
{
	if (m_processingTextures.empty())
		return;

	// 等待主线程 读完成
	m_fenceSync.WriteBegin(m_pCmdQueue.Get());

	m_pCmdAllocator->Reset();
	m_pCmdList->Reset(m_pCmdAllocator.Get(), nullptr);
	NX12Util::BeginEvent(m_pCmdList.Get(), "VT Batch Pass");

	ID3D12DescriptorHeap* heaps[] = { m_pShVisDescHeap->GetDescriptorHeap() };
	m_pCmdList->SetDescriptorHeaps(1, heaps);
	m_pCmdList->SetComputeRootSignature(m_pRootSig.Get());
	m_pCmdList->SetPipelineState(m_pCSO.Get());

	// 按Init里说的顺序绑定Table描述符
	// space0
	m_pBaseColor2DArray->SetResourceState(m_pCmdList.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_pVTPhysicalPage0->SetResourceState(m_pCmdList.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pVTPhysicalPage1->SetResourceState(m_pCmdList.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_pShVisDescHeap->PushFluid(m_pBaseColor2DArray->GetSRV(0));
	m_pShVisDescHeap->PushFluid(m_pVTPhysicalPage0->GetUAV(0));
	m_pShVisDescHeap->PushFluid(m_pVTPhysicalPage1->GetUAV(0));

	// 获取偏移位置（临时）（TODO：真正的偏移位置需要从VT系统获取，现在只是临时测试 ）
	auto getTempPageXY = []() -> Int2 {
		static int idx = 0;
		int x = idx & 31;
		int y = (idx >> 5) & 31;
		x <<= 8;
		y <<= 8;
		idx = (idx + 1) & 1023;
		return Int2(x, y);
		};

	// space1
	int viewCnt = 0;
	int cbCnt = 0;
	for (auto& it = m_processingTextures.begin(); it != m_processingTextures.end();)
	{
		// 无论cb还是view 处理数量超对应上限，都不要再传了
		if (cbCnt >= g_VTConfig.MaxProcessingBatchesAtOnce || viewCnt >= g_VTConfig.MaxRequestTerrainViewsPerUpdate) break;

		auto& task = *it;
		task.pHeightMap->WaitLoadingTexturesFinish();
		task.pSplatMap->WaitLoadingTexturesFinish();

		task.pHeightMap->SetResourceState(m_pCmdList.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		task.pSplatMap->SetResourceState(m_pCmdList.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		m_pShVisDescHeap->PushFluid(task.pHeightMap->GetSRV(0)); // t0, t2, ...
		m_pShVisDescHeap->PushFluid(task.pSplatMap->GetSRV(0)); // t1, t3, ...

		auto& cbData = m_cbVTBatchData[cbCnt];
		cbData.TileWorldPos = task.TileWorldPos;
		cbData.TileWorldSize = task.TileWorldSize;
		cbData.VTPageOffset = getTempPageXY();

		cbCnt++;
		viewCnt += 2;

		// 等GPU队列完成后，才能删，所以要记录GPU完成时的fence
		NXVTTexTaskWithFence gpuFinishTask;
		gpuFinishTask.task = task;
		gpuFinishTask.fenceValue = m_fenceValueSubmit + 1; 
		m_waitGPUFinishTextures.push_back(gpuFinishTask);
		it = m_processingTextures.erase(it);
	}
	m_cbVTBatch.Set(m_cbVTBatchData);

	// space1 补满剩余
	for (; viewCnt < g_VTConfig.MaxRequestTerrainViewsPerUpdate; viewCnt++)
		m_pShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());

	// space2: todo, 贴花的内容还没想好，先放一放
	for (int i = 0; i < g_VTConfig.MaxRequestDecalViewsPerUpdate; i++)
		m_pShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandles = m_pShVisDescHeap->Submit();
	m_pCmdList->SetComputeRootDescriptorTable(0, gpuHandles);

	m_cbVTConfigData.BakeTileNum = cbCnt; // 记录本次烘焙实际使用几个Tile
	m_cbVTConfig.Set(m_cbVTConfigData);

	m_pCmdList->SetComputeRootConstantBufferView(1, m_cbVTConfig.CurrentGPUAddress());
	m_pCmdList->SetComputeRootConstantBufferView(2, m_cbVTBatch.CurrentGPUAddress());

	// Dispatch：XY=单个tile像素，Z=具体哪个tile。numThreads(8,8,1)
	int dispatchXY = (g_VTConfig.PhysicalPageTileSize + 7) / 8;
	m_pCmdList->Dispatch(dispatchXY, dispatchXY, cbCnt);

	NX12Util::EndEvent(m_pCmdList.Get());

	m_pCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(1, cmdLists);

	// fence记录 写完成
	m_fenceSync.WriteEnd(m_pCmdQueue.Get());

	// 提交专用一个独立fence
	m_fenceValueSubmit++;
	m_pCmdQueue->Signal(m_pFenceSubmit.Get(), m_fenceValueSubmit);

	if (m_pFenceSubmit->GetCompletedValue() < m_fenceValueSubmit)
	{
		m_pFenceSubmit->SetEventOnCompletion(m_fenceValueSubmit, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);

		// 删除已处理完的纹理包
		m_waitGPUFinishTextures.clear();
	}
}

void NXVirtualTextureStreaming::AddTexLoadTask(const NXVTInfoTask& task)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 这一步从磁盘读对应的位置的纹理效果，目的是把splatmap、（将来还有heightmap）、decal都加载到内存，形成 一个data包
	// 然后VT烘焙管线再使用每个data包做自己的drawcall，画出真正的纹理来
	m_infoTasks.push_back(task);
}
