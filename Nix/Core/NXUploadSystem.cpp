#include "NXUploadSystem.h"

NXUploadRingBuffer::NXUploadRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize):
	m_pDevice(pDevice),
	m_size(bufferSize),
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

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pResource));
	m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));
}

NXUploadRingBuffer::~NXUploadRingBuffer()
{
	m_pResource->Unmap(0, nullptr);
	m_pResource->Release();
	m_pResource = nullptr;
}

bool NXUploadRingBuffer::CanAlloc(uint32_t byteSize)
{
	// byteSize ���ֽڶ��봦����DX12���������ݶ��뷽ʽΪ׼������С��512�ֽڣ�
	byteSize = (byteSize + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

	// case 1: ed �� st �ĺ���(<) ��û������(=)
	if (m_usedStart <= m_usedEnd)
	{
		// case 1.1. �����µ�ַ�Ժ���Ȼû�дﵽ��β
		if (m_usedEnd + byteSize < m_size)
		{
			return true;
		}
		// case 1.2. �����µ�ַ�Ժ�ᳬ����β���ĳ��ڻ�ͷ������
		else
		{
			return byteSize <= m_usedStart;
		}
	}
	// case 2: ed �� st ��ǰ��(>)
	else // if (m_usedEnd < m_usedStart)
	{
		return m_usedEnd + byteSize < m_usedStart;
	}
}

bool NXUploadRingBuffer::Build(uint32_t byteSize, NXUploadTask& oTask)
{
	NXPrint::Write(0, "BuildTask(Begin), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);

	// byteSize ���ֽڶ��봦����DX12���������ݶ��뷽ʽΪ׼������С��512�ֽڣ�
	byteSize = (byteSize + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

	// case 0. ע�⣺Start == End ֻ��ʾ������RingBufferΪ��
	// ���仰˵ RingBuffer �ǡ����������������ġ����ڴ�״̬Ƶ���仯�ķ��������ԣ�Ӱ�첻��

	// case 1: ed �� st �ĺ���(<) ��û������(=)
	if (m_usedStart <= m_usedEnd)
	{
		// case 1.1. �����µ�ַ�Ժ���Ȼû�дﵽ��β
		if (m_usedEnd + byteSize < m_size)
		{
			oTask.ringPos = m_usedEnd;
			m_usedEnd += byteSize;
		}
		// case 1.2. �����µ�ַ�Ժ�ᳬ����β���ĳ��ڻ�ͷ������
		else 
		{
			// ����Ƿ����㹻�ռ�
			if (byteSize <= m_usedStart)
			{
				oTask.ringPos = 0;
				m_usedEnd = byteSize;
			}
			else
			{
				// ʣ��ռ䲻��˵�����䲻�ˣ�ɶ������
				throw std::exception("RingBuffer is full!");
				return false;
			}
		}
	}
	// case 2: ed �� st ��ǰ��(>)
	else // if (m_usedEnd < m_usedStart)
	{
		// case 2.1. �����µ�ַ֮����Ȼû�г���st
		if (m_usedEnd + byteSize < m_usedStart)
		{
			oTask.ringPos = m_usedEnd;
			m_usedEnd += byteSize;
		}
		// case 2.2. �����µ�ַ֮�󣬳�����st��û��ʣ��ռ��ˣ�
		else
		{
			throw std::exception("RingBuffer is full!");
			return false;
		}
	}

	// ���ߵ����ﶼ�Ƿ���ɹ������
	oTask.byteSize = byteSize;
	oTask.fenceValue = UINT64_MAX;
	NXPrint::Write(0, "BuildTask(End  ), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);
	return true;
}

void NXUploadRingBuffer::Finish(const NXUploadTask& task)
{
	// ������ɺ�ֻ��Ҫ��usedStart��ǰ�ƶ�����
	m_usedStart = task.ringPos + task.byteSize;

	// �� BuildTask() case 1.2�����������β����Ҫ�ػ�
	m_usedStart %= m_size;
}

NXUploadSystem::NXUploadSystem(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_ringBuffer(pDevice, 64 * 1024 * 1024) // 64MB ring buffer.
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCmdQueue));

	m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	m_pFence->SetName(L"NXUploadSystem Fence");

	for (int i = 0; i < TASK_NUM; i++)
	{
		auto& task = m_tasks[i];
		m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&task.pCmdAllocator));
		m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, task.pCmdAllocator, nullptr, IID_PPV_ARGS(&task.pCmdList));
		task.pCmdList->SetName(std::wstring(L"UploadTask" + std::to_wstring(i)).c_str());
		task.pCmdList->Close();
	}
}

NXUploadSystem::~NXUploadSystem()
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

	for (auto& task : m_tasks)
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

bool NXUploadSystem::BuildTask(int byteSize, NXUploadContext& taskResult)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	// ��������������ʱ�������ȴ�
	// update() ÿ���һ�����񣬾ͻ�notify_one()������һ��������������ȴ����̣߳�����еĻ���
	m_condition.wait(lock, [this, byteSize]() {
		bool taskOK = m_taskUsed < TASK_NUM;
		bool ringBufferOK = m_ringBuffer.CanAlloc(byteSize);
		return taskOK && ringBufferOK;
		});

	uint32_t idx = (m_taskStart + m_taskUsed) % TASK_NUM;
	auto& task = m_tasks[idx];
	if (m_ringBuffer.Build(byteSize, task))
	{
		m_taskUsed++;

		task.pCmdAllocator->Reset();
		task.pCmdList->Reset(task.pCmdAllocator, nullptr);

		taskResult.pOwner = &task;
		taskResult.pResource = m_ringBuffer.GetResource();
		taskResult.pResourceData = m_ringBuffer.GetResourceMappedData();
		taskResult.pResourceOffset = task.ringPos;

		return true;
	}

	return false;
}

void NXUploadSystem::FinishTask(const NXUploadContext& result, const std::function<void()>& pCallBack)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	NXUploadTask* task = result.pOwner;
	task->pCallback = pCallBack;

	task->pCmdList->Close();
	ID3D12CommandList* cmdLists[] = { task->pCmdList };
	m_pCmdQueue->ExecuteCommandLists(1, cmdLists);
	
	m_fenceValue++;
	m_frameFenceValue[MultiFrameSets::swapChainIndex] = m_fenceValue;
	m_pCmdQueue->Signal(m_pFence, m_fenceValue); // ��֪GPU ����ִ�����ʱ m_pFence���³�value

	task->fenceValue = m_fenceValue; 
}

void NXUploadSystem::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (m_taskUsed)
	{
		auto& task = m_tasks[m_taskStart];

		// �ȴ�GPU����������
		if (m_pFence->GetCompletedValue() >= task.fenceValue)
		{
			if (task.pCallback)
				task.pCallback(); // ������ɺ�callback

			// ������ɣ�������Դ
			m_ringBuffer.Finish(task);

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
