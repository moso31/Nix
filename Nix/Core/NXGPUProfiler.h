#pragma once
#include "BaseDefs/DX12.h"
#include <string>
#include <vector>
#include <unordered_map>

// GPU Profiler 用于测量 RenderGraph 每个 Pass 的 GPU 执行时间
// 使用 D3D12 Timestamp Query 实现精确的 GPU 时间测量

struct NXGPUProfileResult
{
	std::string passName;
	double timeMs;			// 毫秒
	uint64_t startTick;
	uint64_t endTick;
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

	// 获取上一帧的 profiling 结果（因为 GPU/CPU 异步，需要延迟一帧读取）
	const std::vector<NXGPUProfileResult>& GetLastFrameResults() const { return m_lastFrameResults; }

	// 获取上一帧总 GPU 时间
	double GetLastFrameTotalTimeMs() const { return m_lastFrameTotalTimeMs; }

	// 是否启用 profiling
	void SetEnabled(bool enabled) { m_bEnabled = enabled; }
	bool IsEnabled() const { return m_bEnabled; }

private:
	void ResolveTimestamps();

private:
	bool m_bEnabled = false;
	bool m_bInitialized = false;

	ID3D12Device* m_pDevice = nullptr;
	ID3D12CommandQueue* m_pCmdQueue = nullptr;

	// Query Heap 用于存储 GPU 时间戳
	ComPtr<ID3D12QueryHeap> m_pQueryHeap;
	
	// Readback Buffer 用于从 GPU 读取时间戳数据
	ComPtr<ID3D12Resource> m_pReadbackBuffer;

	// 时间戳频率（每秒多少 tick）
	uint64_t m_timestampFrequency = 0;

	// 当前帧的 query 数量
	uint32_t m_currentQueryIndex = 0;
	uint32_t m_maxQueries = 256;

	// 当前帧正在 profiling 的 pass 名称
	std::string m_currentPassName;

	// 当前帧的 pass 名称列表（按 query 顺序）
	std::vector<std::string> m_currentFramePassNames;

	// 上一帧的 profiling 结果
	std::vector<NXGPUProfileResult> m_lastFrameResults;
	double m_lastFrameTotalTimeMs = 0.0;

	// 双缓冲：记录上一帧的 query 数量，用于 resolve
	uint32_t m_lastFrameQueryCount = 0;
};

// 全局 GPU Profiler 实例
extern NXGPUProfiler* g_pGPUProfiler;
