#include "NXGPUProfiler.h"
#include "NXGlobalDefinitions.h"
#include <algorithm>

NXGPUProfiler* g_pGPUProfiler = nullptr;

NXGPUProfiler::NXGPUProfiler()
{
}

NXGPUProfiler::~NXGPUProfiler()
{
	Release();
}

void NXGPUProfiler::Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue, uint32_t maxQueries)
{
	m_pDevice = pDevice;
	m_pCmdQueue = pCmdQueue;
	m_maxQueries = maxQueries;

	// 获取时间戳频率
	HRESULT hr = pCmdQueue->GetTimestampFrequency(&m_timestampFrequency);
	if (FAILED(hr))
	{
		// 某些 GPU 可能不支持时间戳查询
		m_bEnabled = false;
		return;
	}

	// 创建 Query Heap
	// 每个 pass 需要 2 个 query（开始和结束），所以 heap 大小是 maxQueries * 2
	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	queryHeapDesc.Count = maxQueries * 2;
	queryHeapDesc.NodeMask = 0;

	hr = pDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_pQueryHeap));
	if (FAILED(hr))
	{
		m_bEnabled = false;
		return;
	}

	// 创建 Readback Buffer
	// 每个时间戳是 uint64_t (8 bytes)
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = maxQueries * 2 * sizeof(uint64_t);
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pReadbackBuffer));

	if (FAILED(hr))
	{
		m_pQueryHeap.Reset();
		m_bEnabled = false;
		return;
	}

	m_bInitialized = true;
}

void NXGPUProfiler::Release()
{
	m_pQueryHeap.Reset();
	m_pReadbackBuffer.Reset();
	m_bInitialized = false;
}

void NXGPUProfiler::BeginFrame()
{
	if (!m_bEnabled || !m_bInitialized)
		return;

	// 在开始新一帧之前，先读取上一帧的结果
	ResolveTimestamps();

	// 重置当前帧的状态
	m_lastFrameQueryCount = m_currentQueryIndex;
	m_currentQueryIndex = 0;
	m_currentFramePassNames.clear();
}

void NXGPUProfiler::BeginPass(ID3D12GraphicsCommandList* pCmdList, const std::string& passName)
{
	if (!m_bEnabled || !m_bInitialized)
		return;

	if (m_currentQueryIndex >= m_maxQueries)
		return; // 超过最大 query 数量

	m_currentPassName = passName;
	m_currentFramePassNames.push_back(passName);

	// 写入开始时间戳
	uint32_t queryIndex = m_currentQueryIndex * 2;
	pCmdList->EndQuery(m_pQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
}

void NXGPUProfiler::EndPass(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bEnabled || !m_bInitialized)
		return;

	if (m_currentQueryIndex >= m_maxQueries)
		return;

	// 写入结束时间戳
	uint32_t queryIndex = m_currentQueryIndex * 2 + 1;
	pCmdList->EndQuery(m_pQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);

	m_currentQueryIndex++;
}

void NXGPUProfiler::EndFrame(ID3D12GraphicsCommandList* pCmdList)
{
	if (!m_bEnabled || !m_bInitialized)
		return;

	if (m_currentQueryIndex == 0)
		return;

	// 将时间戳数据从 Query Heap 拷贝到 Readback Buffer
	pCmdList->ResolveQueryData(
		m_pQueryHeap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		0,
		m_currentQueryIndex * 2,
		m_pReadbackBuffer.Get(),
		0);
}

void NXGPUProfiler::ResolveTimestamps()
{
	if (!m_bEnabled || !m_bInitialized)
		return;

	if (m_lastFrameQueryCount == 0)
		return;

	// 读取 Readback Buffer 中的时间戳数据
	uint64_t* pTimestampData = nullptr;
	D3D12_RANGE readRange = { 0, m_lastFrameQueryCount * 2 * sizeof(uint64_t) };
	
	HRESULT hr = m_pReadbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pTimestampData));
	if (FAILED(hr))
		return;

	m_lastFrameResults.clear();
	m_lastFrameTotalTimeMs = 0.0;

	// 解析每个 pass 的时间
	for (uint32_t i = 0; i < m_lastFrameQueryCount && i < m_currentFramePassNames.size(); i++)
	{
		uint64_t startTick = pTimestampData[i * 2];
		uint64_t endTick = pTimestampData[i * 2 + 1];

		// 转换为毫秒
		double timeMs = (endTick - startTick) * 1000.0 / m_timestampFrequency;

		NXGPUProfileResult result;
		result.passName = m_currentFramePassNames[i];
		result.timeMs = timeMs;
		result.startTick = startTick;
		result.endTick = endTick;

		m_lastFrameResults.push_back(result);
		m_lastFrameTotalTimeMs += timeMs;
	}

	D3D12_RANGE writeRange = { 0, 0 }; // 我们没有写入，所以 range 为空
	m_pReadbackBuffer->Unmap(0, &writeRange);

	// 更新历史数据
	UpdateHistory();
	TrimHistory();
}

void NXGPUProfiler::UpdateHistory()
{
	auto now = std::chrono::steady_clock::now();
	for (const auto& result : m_lastFrameResults)
	{
		TimedSample sample;
		sample.timeMs = result.timeMs;
		sample.timestamp = now;
		m_passTimeHistory[result.passName].push_back(sample);
	}
}

void NXGPUProfiler::TrimHistory()
{
	auto now = std::chrono::steady_clock::now();
	auto cutoff = now - std::chrono::milliseconds(static_cast<int64_t>(m_historyDuration * 1000.0f));

	for (auto& [passName, samples] : m_passTimeHistory)
	{
		while (!samples.empty() && samples.front().timestamp < cutoff)
		{
			samples.pop_front();
		}
	}

	// 清理空的条目
	for (auto it = m_passTimeHistory.begin(); it != m_passTimeHistory.end(); )
	{
		if (it->second.empty())
			it = m_passTimeHistory.erase(it);
		else
			++it;
	}
}

NXGPUProfileStats NXGPUProfiler::GetPassStats(const std::string& passName, float durationSeconds) const
{
	NXGPUProfileStats stats;

	auto it = m_passTimeHistory.find(passName);
	if (it == m_passTimeHistory.end() || it->second.empty())
		return stats;

	auto now = std::chrono::steady_clock::now();
	auto cutoff = now - std::chrono::milliseconds(static_cast<int64_t>(durationSeconds * 1000.0f));

	double minMs = std::numeric_limits<double>::max();
	double maxMs = 0.0;
	double totalMs = 0.0;
	uint32_t count = 0;

	for (const auto& sample : it->second)
	{
		if (sample.timestamp >= cutoff)
		{
			minMs = std::min(minMs, sample.timeMs);
			maxMs = std::max(maxMs, sample.timeMs);
			totalMs += sample.timeMs;
			count++;
		}
	}

	if (count > 0)
	{
		stats.minMs = minMs;
		stats.maxMs = maxMs;
		stats.avgMs = totalMs / count;
		stats.sampleCount = count;
	}

	return stats;
}
