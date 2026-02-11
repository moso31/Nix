/*
 * NXVTDebugger — 虚拟纹理系统的统一调试工具
 *
 * 整合所有 VT 模块的调试输出 (之前分散在 VTLog / m_enableDebugPrint / printf 中)，
 * 提供分类开关和统一的日志写入方式。
 *
 * 使用方式:
 *   NXVTDebugger& dbg = NXVTDebugger::GetInstance();
 *   dbg.SetEnabled(NXVTDebugCategory::RenderPass, true);
 *   dbg.Log(NXVTDebugCategory::RenderPass, "[BakePhysicalPage] PageID: (%d, %d)\n", px, py);
 *
 * IndirectTexture CPU-side 模拟 (原 NXVTIndirectTextureTracker) 也集成在此类中，
 * 通过 NXVTDebugCategory::Tracker 控制其日志输出。
 */

#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <vector>
#include <array>
#include "BaseDefs/Math.h"
#include "NXVirtualTextureCommon.h"

// 前向声明
struct CBufferRemoveSector;
struct CBufferMigrateSector;

// ---- 调试分类 ----
enum class NXVTDebugCategory : uint32_t
{
	RenderPass,		// RegisterXxxPass 中的日志 (原 m_enableDebugPrint 控制的内容)
	LRUCache,		// LRU Insert / Replace / Update 日志
	SectorUpdate,	// Sector 新增/移除/迁移的警告信息
	Tracker,		// IndirectTexture CPU 模拟镜像的逐像素日志
	Count
};

class NXVTDebugger
{
public:
	static NXVTDebugger& GetInstance();

	// ---- 全局开关 ----
	void SetGlobalEnabled(bool enabled) { m_globalEnabled = enabled; }
	bool IsGlobalEnabled() const { return m_globalEnabled; }

	// ---- 分类开关 ----
	void SetEnabled(NXVTDebugCategory cat, bool enabled);
	bool IsEnabled(NXVTDebugCategory cat) const;

	// ---- 日志输出 ----
	// 自动检查 全局 + 分类 开关，满足时写入文件。
	void Log(NXVTDebugCategory cat, const char* fmt, ...);

	// ---- 设置日志文件路径 (默认 "D:/VTDebug.txt") ----
	void SetLogFilePath(const char* path);

	// ---- 清空日志文件 ----
	void ClearLogFile();

	// ======================================================
	// IndirectTexture CPU-side 模拟 (原 NXVTIndirectTextureTracker)
	// ======================================================

	static constexpr int IT_MIP_LEVELS = 11;
	static constexpr int IT_BASE_SIZE = 2048;
	static constexpr uint16_t IT_EMPTY = 0xFFFF;

	void TrackerClear();
	void TrackerSimulateRemove(const CBufferRemoveSector& removeData);
	void TrackerSimulateMigrate(const CBufferMigrateSector& migrateData);
	void TrackerSimulateUpdateIndex(int physPageIndex, const Int2& pageID, int gpuMip);
	uint16_t TrackerGetPixel(int mip, int x, int y) const;

private:
	NXVTDebugger();
	~NXVTDebugger() = default;
	NXVTDebugger(const NXVTDebugger&) = delete;
	NXVTDebugger& operator=(const NXVTDebugger&) = delete;

	void LogV(const char* fmt, va_list args);

	int TrackerMipSize(int mip) const { return IT_BASE_SIZE >> mip; }
	int TrackerMipOffset(int mip) const;

private:
	bool m_globalEnabled = false;
	std::array<bool, (size_t)NXVTDebugCategory::Count> m_categoryEnabled{};

	char m_logFilePath[260] = "D:/VTDebug.txt";

	// IndirectTexture CPU mirror data
	std::vector<uint16_t> m_trackerData;
};
