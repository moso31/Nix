#pragma once
#include "BaseDefs/DX12.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>

// GPU Profiler 用于测量 RenderGraph 每个 Pass 的 GPU 执行时间
// 使用 D3D12 Timestamp Query 实现精确的 GPU 时间测量
//
// 三缓冲设计：每帧的 passNames、queryCount、readbackBuffer 绑定在一起，
// 读取时始终读 2 帧前（由 Fence 保证 GPU 已完成）的数据，
// 避免 passNames 与 timestamps 来自不同帧导致的错位问题。

struct NXGPUProfileResult
{
	std::string passName;
	double timeMs;			// 毫秒
	uint64_t startTick;
	uint64_t endTick;
};

struct NXGPUProfileStats
{
	double minMs = 0.0;
	double maxMs = 0.0;
	double avgMs = 0.0;
	uint32_t sampleCount = 0;
};

class NXGPUProfiler
{
public:
	NXGPUProfiler();
	~NXGPUProfiler();

	void Init(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue, uint32_t maxQueries = 256);
	void Release();

	// 每帧开始时调用，准备新一帧的 profiling
	void BeginFrame();

	// 在 pass 执行前后调用
	void BeginPass(ID3D12GraphicsCommandList* pCmdList, const std::string& passName);
	void EndPass(ID3D12GraphicsCommandList* pCmdList);

	// 帧结束时调用，解析时间戳数据
	void EndFrame(ID3D12GraphicsCommandList* pCmdList);

	// 获取已完成帧的 profiling 结果（延迟 2 帧，确保 GPU 已完成）
	const std::vector<NXGPUProfileResult>& GetLastFrameResults() const { return m_lastFrameResults; }

	// 获取已完成帧总 GPU 时间
	double GetLastFrameTotalTimeMs() const { return m_lastFrameTotalTimeMs; }

	// 获取指定 pass 在指定时间范围内的统计数据（最小/最大/平均）
	NXGPUProfileStats GetPassStats(const std::string& passName, float durationSeconds) const;

	// 历史数据持续时间（秒）
	float GetHistoryDuration() const { return m_historyDuration; }
	void SetHistoryDuration(float seconds) { m_historyDuration = seconds; }

	// 是否启用 profiling
	void SetEnabled(bool enabled) { m_bEnabled = enabled; }
	bool IsEnabled() const { return m_bEnabled; }

private:
	void ResolveTimestamps();
	void UpdateHistory();
	void TrimHistory();

private:
	bool m_bEnabled = false;
	bool m_bInitialized = false;

	ID3D12Device* m_pDevice = nullptr;
	ID3D12CommandQueue* m_pCmdQueue = nullptr;

	// Query Heap 用于存储 GPU 时间戳
	ComPtr<ID3D12QueryHeap> m_pQueryHeap;

	// 时间戳频率（每秒多少 tick）
	uint64_t m_timestampFrequency = 0;

	uint32_t m_maxQueries = 256;

	// 三缓冲的每帧 Profiler 数据
	// passNames、queryCount、readbackBuffer 绑定在一起，保证读取时三者来自同一帧
	struct PerFrameData
	{
		ComPtr<ID3D12Resource> pReadbackBuffer;
		std::vector<std::string> passNames;
		uint32_t queryCount = 0;
	};
	PerFrameData m_frameData[MultiFrameSets_swapChainCount];

	// 当前帧正在录制的 query 索引
	uint32_t m_currentQueryIndex = 0;

	// 当前帧总计数（用于追踪启动前几帧还没有可读数据）
	uint64_t m_totalFrameCount = 0;

	// 解析后的结果（供 GUI 读取）
	std::vector<NXGPUProfileResult> m_lastFrameResults;
	double m_lastFrameTotalTimeMs = 0.0;

	// 历史数据追踪（用于详细统计）
	struct TimedSample
	{
		double timeMs;
		std::chrono::steady_clock::time_point timestamp;
	};
	std::unordered_map<std::string, std::deque<TimedSample>> m_passTimeHistory;
	float m_historyDuration = 5.0f; // 保留最近多少秒的历史数据
};

// 全局 GPU Profiler 实例
extern NXGPUProfiler* g_pGPUProfiler;
