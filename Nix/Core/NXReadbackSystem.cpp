#include "NXReadbackSystem.h"
#include "NXGlobalDefinitions.h"

NXReadbackRingBuffer::NXReadbackRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize) :
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
	
	// ring buffer
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
	m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));
}

NXReadbackRingBuffer::~NXReadbackRingBuffer()
{
	m_pResource->Unmap(0, nullptr);
	m_pResource->Release();
	m_pResource = nullptr;
}

bool NXReadbackRingBuffer::CanAlloc(uint32_t byteSize)
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

bool NXReadbackRingBuffer::Build(uint32_t byteSize, NXReadbackTask& oTask)
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
	NXPrint::Write(0, "BuildTask(End  ), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);
	return true;
}

void NXReadbackRingBuffer::Finish(const NXReadbackTask& task)
{
	// ������ɺ�ֻ��Ҫ��usedStart��ǰ�ƶ�����
	m_usedStart = task.ringPos + task.byteSize;

	// �� BuildTask() case 1.2�����������β����Ҫ�ػ�
	m_usedStart %= m_size;
}

NXReadbackSystem::NXReadbackSystem(ID3D12Device* pDevice) :
	m_pDevice(pDevice),
	m_ringBuffer(pDevice, 64 * 1024 * 1024) // 64MB ring buffer.
{
}

NXReadbackSystem::~NXReadbackSystem()
{
}

bool NXReadbackSystem::BuildTask(int byteSize, NXReadbackContext& taskResult)
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

		taskResult.pOwner = &task;
		taskResult.pResource = m_ringBuffer.GetResource();
		taskResult.pResourceData = m_ringBuffer.GetResourceMappedData();
		taskResult.pResourceOffset = task.ringPos;

		m_pendingTask.push_back(&task);

		return true;
	}

	return false;
}

void NXReadbackSystem::FinishTask(const NXReadbackContext& result, const std::function<void()>& pCallBack)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	NXReadbackTask* task = result.pOwner;
	task->pCallback = pCallBack;
}

void NXReadbackSystem::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (m_taskUsed)
	{
		auto& task = m_tasks[m_taskStart];

		// �ȴ�GPU����������
		if (NXGlobalDX::s_globalfence->GetCompletedValue() >= task.mainRenderFenceValue)
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

void NXReadbackSystem::UpdatePendingTaskFenceValue(uint64_t mainRenderFenceValue)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto task : m_pendingTask)
	{
		task->mainRenderFenceValue = mainRenderFenceValue;
	}
	m_pendingTask.clear();
}
