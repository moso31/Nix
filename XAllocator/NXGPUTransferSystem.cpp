#include "NXGPUTransferSystem.h"

NXTransferTask::NXTransferTask()
{
}

NXRingBuffer::NXRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize, NXTransferType type):
	m_pDevice(pDevice),
	m_size(bufferSize),
	m_type(type),
	m_usedStart(0),
	m_usedEnd(0)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = m_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	assert(type != NXTransferType::None);
	if (type == NXTransferType::Upload)
	{
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pResource));
		m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));
	}
	else
	{
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
		m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));
	}
}

NXRingBuffer::~NXRingBuffer()
{
	m_pResource->Unmap(0, nullptr);
	m_pResource->Release();
	m_pResource = nullptr;
}

bool NXRingBuffer::CanAlloc(uint32_t byteSize)
{
	// byteSize 做字节对齐处理，以DX12的纹理数据对齐方式为准（不得小于512字节）
	byteSize = (byteSize + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

	// case 1: ed 在 st 的后面(<) 或没有任务(=)
	if (m_usedStart <= m_usedEnd)
	{
		// case 1.1. 加入新地址以后，依然没有达到环尾
		if (m_usedEnd + byteSize < m_size)
		{
			return true;
		}
		// case 1.2. 加入新地址以后会超过环尾（改成在环头创建）
		else
		{
			return byteSize <= m_usedStart;
		}
	}
	// case 2: ed 在 st 的前面(>)
	else // if (m_usedEnd < m_usedStart)
	{
		return m_usedEnd + byteSize < m_usedStart;
	}
}

bool NXRingBuffer::Build(uint32_t byteSize, NXTransferTask& oTask)
{
	NXPrint::Write(0, "BuildTask(Begin), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);

	// byteSize 做字节对齐处理，以DX12的纹理数据对齐方式为准（不得小于512字节）
	byteSize = (byteSize + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

	// case 0. 注意：Start == End 只表示：整个RingBuffer为空
	// 换句话说 RingBuffer 是【不允许填满！】的。对内存状态频繁变化的分配器而言，影响不大

	// case 1: ed 在 st 的后面(<) 或没有任务(=)
	if (m_usedStart <= m_usedEnd)
	{
		// case 1.1. 加入新地址以后，依然没有达到环尾
		if (m_usedEnd + byteSize < m_size)
		{
			oTask.ringPos = m_usedEnd;
			m_usedEnd += byteSize;
		}
		// case 1.2. 加入新地址以后会超过环尾（改成在环头创建）
		else 
		{
			// 检测是否有足够空间
			if (byteSize <= m_usedStart)
			{
				oTask.ringPos = 0;
				m_usedEnd = byteSize;
			}
			else
			{
				// 剩余空间不够说明分配不了，啥都别做
				throw std::exception("RingBuffer is full!");
				return false;
			}
		}
	}
	// case 2: ed 在 st 的前面(>)
	else // if (m_usedEnd < m_usedStart)
	{
		// case 2.1. 加入新地址之后，依然没有超过st
		if (m_usedEnd + byteSize < m_usedStart)
		{
			oTask.ringPos = m_usedEnd;
			m_usedEnd += byteSize;
		}
		// case 2.2. 加入新地址之后，超过了st（没有剩余空间了）
		else
		{
			throw std::exception("RingBuffer is full!");
			return false;
		}
	}

	// 能走到这里都是分配成功的情况
	oTask.byteSize = byteSize;
	oTask.fenceValue = UINT64_MAX;
	oTask.type = m_type;
	oTask.pRingBuffer = this;
	NXPrint::Write(0, "BuildTask(End  ), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);
	return true;
}

void NXRingBuffer::Finish(const NXTransferTask& task)
{
	// 任务完成后，只需要将usedStart向前移动即可
	m_usedStart = task.ringPos + task.byteSize;

	// 见 BuildTask() case 1.2，如果超过环尾，需要回环
	m_usedStart %= m_size;
}

NXGPUTransferSystem::NXGPUTransferSystem(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_ringBufferUpload(pDevice, 64 * 1024 * 1024, NXTransferType::Upload), // 64MB ring buffer.
	m_ringBufferReadback(pDevice, 64 * 1024 * 1024, NXTransferType::Readback) // 64MB ring buffer.
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCmdQueue));

	m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	for (int i = 0; i < TASK_NUM; i++)
	{
		auto& task = m_transferTask[i];
		m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&task.pCmdAllocator));
		m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, task.pCmdAllocator, nullptr, IID_PPV_ARGS(&task.pCmdList));
		task.pCmdList->SetName(std::wstring(L"UploadTask" + std::to_wstring(i)).c_str());
		task.pCmdList->Close();
	}
}

NXGPUTransferSystem::~NXGPUTransferSystem()
{
	if (m_pCmdQueue)
	{
		m_pCmdQueue->Release();
		m_pCmdQueue = nullptr;
	}

	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}

	for (auto& task : m_transferTask)
	{
		if (task.pCmdAllocator)
		{
			task.pCmdAllocator->Release();
			task.pCmdAllocator = nullptr;
		}

		if (task.pCmdList)
		{
			task.pCmdList->Release();
			task.pCmdList = nullptr;
		}
	}
}

bool NXGPUTransferSystem::BuildTask(int byteSize, NXTransferType taskType, NXTransferContext& taskResult)
{
	assert(taskType != NXTransferType::None);

	std::unique_lock<std::mutex> lock(m_mutex);

	NXRingBuffer* ringBuffer = (taskType == NXTransferType::Upload) ?
		&m_ringBufferUpload : &m_ringBufferReadback;

	// 不满足以下条件时，持续等待
	// update() 每完成一个任务，就会notify_one()，唤醒一个正在这里持续等待的线程（如果有的话）
	m_condition.wait(lock, [this, byteSize, ringBuffer]() {
		bool taskOK = m_taskUsed < TASK_NUM;
		bool ringBufferOK = ringBuffer->CanAlloc(byteSize);
		return taskOK && ringBufferOK;
		});

	uint32_t idx = (m_taskStart + m_taskUsed) % TASK_NUM;
	auto& task = m_transferTask[idx];
	if (ringBuffer->Build(byteSize, task))
	{
		m_taskUsed++;

		task.pCmdAllocator->Reset();
		task.pCmdList->Reset(task.pCmdAllocator, nullptr);

		taskResult.pOwner = &task;
		taskResult.pResource = ringBuffer->GetResource();
		taskResult.pResourceData = ringBuffer->GetResourceMappedData();
		taskResult.pResourceOffset = task.ringPos;

		return true;
	}

	return false;
}

void NXGPUTransferSystem::FinishTask(const NXTransferContext& result, const std::function<void()>& pCallBack)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	NXTransferTask* task = result.pOwner;
	task->pCallback = pCallBack;

	task->pCmdList->Close();
	ID3D12CommandList* cmdLists[] = { task->pCmdList };
	m_pCmdQueue->ExecuteCommandLists(1, cmdLists);
	
	m_fenceValue++;
	m_frameFenceValue[MultiFrameSets::swapChainIndex] = m_fenceValue;
	m_pCmdQueue->Signal(m_pFence, m_fenceValue); // 告知GPU 命令执行完成时 m_pFence更新成value

	task->fenceValue = m_fenceValue; 
}

void NXGPUTransferSystem::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (m_taskUsed)
	{
		auto& task = m_transferTask[m_taskStart];

		// 等待GPU侧的任务完成
		if (m_pFence->GetCompletedValue() >= task.fenceValue)
		{
			if (task.pCallback)
				task.pCallback(); // 触发完成后callback

			// 任务完成，回收资源
			if (task.pRingBuffer)
				task.pRingBuffer->Finish(task);

			m_taskStart = (m_taskStart + 1) % TASK_NUM;
			m_taskUsed--;
			task.Reset();

			m_condition.notify_one();
		}
		else
		{
			break;
		}
	}
}
