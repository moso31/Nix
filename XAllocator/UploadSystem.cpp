#include "UploadSystem.h"

using namespace ccmem;

ccmem::UploadRingBuffer::UploadRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize):
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

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	
	m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pResource));
	m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));
}

ccmem::UploadRingBuffer::~UploadRingBuffer()
{
	m_pResource->Unmap(0, nullptr);
	m_pResource->Release();
	m_pResource = nullptr;
}

bool ccmem::UploadRingBuffer::BuildTask(uint32_t byteSize, UploadTask& oTask)
{
	// ע�⣺Start == End ��ʾ����RingBufferΪ��
	// ���仰˵ RingBuffer �� ���������� �ġ����ڴ�״̬Ƶ���仯�ķ��������ԣ�Ӱ�첻��

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
			return false;
		}
	}

	// ���ߵ����ﶼ�Ƿ���ɹ������
	oTask.byteSize = byteSize;
	oTask.fenceValue = UINT64_MAX;
	return true;
}

ccmem::UploadSystem::UploadSystem(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_ringBuffer(UploadRingBuffer(pDevice, 64 * 1024 * 1024)) // 64MB ring buffer.
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCmdQueue));

	m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

	for (auto& task : m_uploadTask)
	{
		m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&task.pCmdAllocator));
		m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, task.pCmdAllocator, nullptr, IID_PPV_ARGS(&task.pCmdList));
		task.pCmdList->Close();
	}
}

ccmem::UploadSystem::~UploadSystem()
{
	if (m_pCmdQueue)
	{
		m_pCmdQueue->Release();
		m_pCmdQueue = nullptr;
	}

	for (auto& task : m_uploadTask)
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

bool ccmem::UploadSystem::BuildTask(int byteSize, UploadTaskContext& taskResult)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_taskUsed > UPLOADTASK_NUM)
	{
		return false;
	}

	uint32_t idx = (m_taskStart + m_taskUsed) % UPLOADTASK_NUM;
	auto& task = m_uploadTask[idx];
	if (m_ringBuffer.BuildTask(byteSize, task))
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

void ccmem::UploadSystem::FinishTask(const UploadTaskContext& result)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	UploadTask* task = result.pOwner;

	task->pCmdList->Close();
	ID3D12CommandList* cmdLists[] = { task->pCmdList };
	m_pCmdQueue->ExecuteCommandLists(1, cmdLists);
	
	m_fenceValue++;
	m_pFence->Signal(m_fenceValue); // GPU fence == N 

	task->fenceValue = m_fenceValue; 
}

void ccmem::UploadSystem::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// ÿ֡���
	bool checkNext = true;

	while (m_taskUsed)
	{
		auto& task = m_uploadTask[m_taskStart];

		// fenceValue �����ü� task.fenceValue ��ͷ�ļ��Ľ���
		// ���� >= ˵��task��GPUһ��Ҳ�Ѿ�ִ�����ˣ����Ա��Ƴ���
		if (m_pFence->GetCompletedValue() >= task.fenceValue)
		{
			m_taskStart = (m_taskStart + 1) % UPLOADTASK_NUM;
			m_taskUsed--;
			task.Reset();
		}
		else
		{
			break;
		}
	}
}
