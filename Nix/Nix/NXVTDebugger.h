/* 注：这段代码是 Vibe coding 的！
 * NXVTDebugger — 虚拟纹理系统的统一调试工具
 *
 * 整合所有 VT 模块的调试输出 (之前分散在 VTLog / m_enableDebugPrint / printf 中)，
 * 通过 32 位位掩码 NXVTDebugBits 控制每个子类别的日志开关。
 *
 * 使用方式:
 *   NXVTDebugger& dbg = NXVTDebugger::GetInstance();
 *   dbg.Enable(VTDBG_BakePhysicalPage | VTDBG_LRUInsert);
 *   dbg.Log(VTDBG_BakePhysicalPage, "[BakePhysicalPage] PageID: (%d, %d)\n", px, py);
 *
 * IndirectTexture CPU-side 模拟器也集成在此类中，
 * 通过 VTDBG_Tracker_All / VTDBG_TrackerXxx 位控制其日志输出。
 */

#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <vector>
#include "BaseDefs/Math.h"
#include "NXVirtualTextureCommon.h"

// 前向声明
struct CBufferRemoveSector;
struct CBufferMigrateSector;

// ---- 调试分类 (位标志) ----
// 使用 32 位掩码，每个类别占 1 bit，可自由组合。
enum NXVTDebugBits : uint32_t
{
	// RenderPass 细分
	VTDBG_UpdateSector2VirtImg			= 1u << 0,
	VTDBG_BakePhysicalPage				= 1u << 1,
	VTDBG_UpdateIndirectTexture			= 1u << 2,
	VTDBG_RemoveIndirectTexture			= 1u << 3,
	VTDBG_MigrateIndirectTexture		= 1u << 4,

	// LRU 细分
	VTDBG_LRUUpdate						= 1u << 5,
	VTDBG_LRUInsert						= 1u << 6,
	VTDBG_LRUReplace					= 1u << 7,
	VTDBG_LRUWarning					= 1u << 8,

	// Sector 细分
	VTDBG_SectorRemoveWarn				= 1u << 9,
	VTDBG_SectorMigrateWarn				= 1u << 10,
	VTDBG_SectorOverflow				= 1u << 11,

	// Tracker 细分
	VTDBG_TrackerClear					= 1u << 12,
	VTDBG_TrackerRemove					= 1u << 13,
	VTDBG_TrackerMigrate				= 1u << 14,
	VTDBG_TrackerUpdateIndex			= 1u << 15,

	// ---- 组合快捷掩码 ----
	VTDBG_RenderPass_All				= VTDBG_UpdateSector2VirtImg | VTDBG_BakePhysicalPage | VTDBG_UpdateIndirectTexture | VTDBG_RemoveIndirectTexture | VTDBG_MigrateIndirectTexture,
	VTDBG_LRU_All						= VTDBG_LRUUpdate | VTDBG_LRUInsert | VTDBG_LRUReplace | VTDBG_LRUWarning,
	VTDBG_Sector_All					= VTDBG_SectorRemoveWarn | VTDBG_SectorMigrateWarn | VTDBG_SectorOverflow,
	VTDBG_Tracker_All					= VTDBG_TrackerClear | VTDBG_TrackerRemove | VTDBG_TrackerMigrate | VTDBG_TrackerUpdateIndex,
	VTDBG_All							= 0xFFFFFFFFu,
	VTDBG_None							= 0u,
};

// 支持位运算组合
inline NXVTDebugBits operator|(NXVTDebugBits a, NXVTDebugBits b) { return (NXVTDebugBits)((uint32_t)a | (uint32_t)b); }
inline NXVTDebugBits operator&(NXVTDebugBits a, NXVTDebugBits b) { return (NXVTDebugBits)((uint32_t)a & (uint32_t)b); }
inline NXVTDebugBits operator~(NXVTDebugBits a) { return (NXVTDebugBits)(~(uint32_t)a); }
inline NXVTDebugBits& operator|=(NXVTDebugBits& a, NXVTDebugBits b) { return a = a | b; }
inline NXVTDebugBits& operator&=(NXVTDebugBits& a, NXVTDebugBits b) { return a = a & b; }

// ---- 输出模式 ----
enum class NXVTDebugOutput : uint8_t
{
	Console,	// printf 输出到 Windows 控制台 (轻量级)
	File,		// 写入到日志文件 (大量日志时使用)
};

#ifdef _DEBUG

class NXVTDebugger
{
public:
	static NXVTDebugger& GetInstance();

	// ---- 启用/禁用标志位 ----
	void Enable(NXVTDebugBits bits)  { m_activeTag |= bits; }
	void Disable(NXVTDebugBits bits) { m_activeTag &= ~bits; }
	void SetTag(NXVTDebugBits tag)   { m_activeTag = tag; }
	NXVTDebugBits GetTag() const     { return m_activeTag; }

	bool IsEnabled(NXVTDebugBits bits) const { return (m_activeTag & bits) != 0; }

	// ---- 输出模式 ----
	void SetOutput(NXVTDebugOutput mode) { m_output = mode; }
	NXVTDebugOutput GetOutput() const { return m_output; }

	// ---- 日志输出 ----
	// 仅当 bits 中任意一位被激活时才写入日志。
	void Log(NXVTDebugBits bits, const char* fmt, ...);

	// ---- 设置日志文件路径 (File 模式下使用，默认 "D:/VTDebug.txt") ----
	void SetLogFilePath(const char* path);

	// ---- 清空日志文件 ----
	void ClearLogFile();

	// ======================================================
	// IndirectTexture CPU-side 模拟
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
	NXVTDebugBits m_activeTag = VTDBG_None;
	NXVTDebugOutput m_output = NXVTDebugOutput::Console;

	char m_logFilePath[260] = "D:/VTDebug.txt";

	// IndirectTexture 在 CPU 一侧的静态数据
	// 仅应在调试时使用
	std::vector<uint16_t> m_trackerData;
};

#else // Release: 所有调试功能编译为空操作，零开销

class NXVTDebugger
{
public:
	static NXVTDebugger& GetInstance() { static NXVTDebugger inst; return inst; }

	void Enable(NXVTDebugBits) {}
	void Disable(NXVTDebugBits) {}
	void SetTag(NXVTDebugBits) {}
	NXVTDebugBits GetTag() const { return VTDBG_None; }
	bool IsEnabled(NXVTDebugBits) const { return false; }

	void SetOutput(NXVTDebugOutput) {}
	NXVTDebugOutput GetOutput() const { return NXVTDebugOutput::Console; }

	void Log(NXVTDebugBits, const char*, ...) {}
	void SetLogFilePath(const char*) {}
	void ClearLogFile() {}

	static constexpr int IT_MIP_LEVELS = 11;
	static constexpr int IT_BASE_SIZE = 2048;
	static constexpr uint16_t IT_EMPTY = 0xFFFF;

	void TrackerClear() {}
	void TrackerSimulateRemove(const CBufferRemoveSector&) {}
	void TrackerSimulateMigrate(const CBufferMigrateSector&) {}
	void TrackerSimulateUpdateIndex(int, const Int2&, int) {}
	uint16_t TrackerGetPixel(int, int, int) const { return IT_EMPTY; }
};

#endif // _DEBUG
