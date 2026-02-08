#include "NXReadbackSystem.h"
#include "NXGlobalDefinitions.h"
#include "NXTexture.h"

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

bool NXReadbackRingBuffer::Build(uint32_t byteSize, NXReadbackTask& oTask)
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
	NXPrint::Write(0, "BuildTask(End  ), usedstart: %d, end: %d\n", m_usedStart, m_usedEnd);
	return true;
}

void NXReadbackRingBuffer::Finish(const NXReadbackTask& task)
{
	// 任务完成后，只需要将usedStart向前移动即可
	m_usedStart = task.ringPos + task.byteSize;

	// 见 BuildTask() case 1.2，如果超过环尾，需要回环
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

	// 不满足以下条件时，持续等待
	// update() 每完成一个任务，就会notify_one()，唤醒一个正在这里持续等待的线程（如果有的话）
	m_condition.wait(lock, [this, byteSize]() {
        return m_taskUsed < TASK_NUM && m_ringBuffer.CanAlloc(byteSize);
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

bool NXReadbackSystem::BuildTask(const Ntr<NXTexture>& pTexture, NXReadbackContext& taskResult)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	auto* pD3DTextureResource = pTexture->GetD3DResource();

    // 获取纹理布局
	// NOTE: 现在只支持对2DArray的mip0进行回读！
    D3D12_RESOURCE_DESC texDesc = pD3DTextureResource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    uint32_t numRows; // 只支持2darray mip0 的回读，所以不需要弄成数组了
    uint64_t rowSizeInBytes;
    uint64_t totalBytes;
	
	uint32_t firstSubresourceIndex = 0;
    m_pDevice->GetCopyableFootprints(&texDesc, firstSubresourceIndex, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

    // 分配RingBuffer空间（totalSize=对齐后的大小）
    uint32_t byteSize = static_cast<uint32_t>(totalBytes);
    
    m_condition.wait(lock, [this, byteSize]() {
        return m_taskUsed < TASK_NUM && m_ringBuffer.CanAlloc(byteSize);
    });

    uint32_t idx = (m_taskStart + m_taskUsed) % TASK_NUM;
    auto& task = m_tasks[idx];
    if (m_ringBuffer.Build(byteSize, task))
    {
        m_taskUsed++;

        // 填充Context
        taskResult.pOwner = &task;
        taskResult.pResource = m_ringBuffer.GetResource();
        taskResult.pResourceData = m_ringBuffer.GetResourceMappedData();
        taskResult.pResourceOffset = task.ringPos;
        
        // 纹理专用信息
        taskResult.type = NXReadbackType::Texture;
        taskResult.footprint = footprint;
        taskResult.footprint.Offset = task.ringPos;  // ringbuffer中的实际偏移
        taskResult.numRows = numRows;
        taskResult.rowSizeInBytes = rowSizeInBytes;

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

		// 等待GPU侧的任务完成
		if (NXGlobalDX::s_globalfence->GetCompletedValue() >= task.mainRenderFenceValue)
		{
			if (task.pCallback)
				task.pCallback(); // 触发完成后callback

			// 任务完成，回收资源
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
