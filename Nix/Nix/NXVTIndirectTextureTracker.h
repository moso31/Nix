/* 2026.2.11
 * 这个类是AI生成的调试类。
 * 作用是在CPU模拟GPU indirectTexture的变化情况。
 * 写这个是因为遇到了物理页错位问题，如果不做一套CPU状态同步机制，问题会非常难查
 */

#pragma once
#include <cstdint>
#include <vector>
#include "BaseDefs/Math.h"
#include "NXVirtualTextureCommon.h"

// 前向声明
struct CBufferRemoveSector;
struct CBufferMigrateSector;

// IndirectTexture 的 CPU 侧镜像（R16_UINT, 2048x2048, 11级mip）。
// 精确模拟 GPU compute shader 对每个像素的操作，
// 并将每次像素变化记录到 D:/1.txt 以便调试。
//
// 此类独立存放在单独文件中，避免污染 VT 主逻辑。
class NXVTIndirectTextureTracker
{
public:
	static constexpr int MIP_LEVELS = 11;
	static constexpr int BASE_SIZE = 2048;
	static constexpr uint16_t EMPTY = 0xFFFF;

	NXVTIndirectTextureTracker();
	~NXVTIndirectTextureTracker() = default;

	// 模拟 GPU "全部清除" pass：将所有 mip 填充为 0xFFFF。
	void Clear();

	// 模拟 CS_Remove：清除区域 [imagePos*imageSize .. +imageSize) 内 mip 0..maxRemoveMip-1 的像素。
	void SimulateRemove(const CBufferRemoveSector& removeData);

	// 模拟 CS_Migrate：在两个区域之间跨 mip 级别移动像素值。
	void SimulateMigrate(const CBufferMigrateSector& migrateData);

	// 模拟 UpdateIndirectTexture CS：写入或清除指定 mip 级别的单个像素。
	void SimulateUpdateIndex(int physPageIndex, const Int2& pageID, int gpuMip);

	// 从 CPU 镜像中读取指定 mip 的 (x, y) 像素值。
	uint16_t GetPixel(int mip, int x, int y) const;

private:
	int MipSize(int mip) const { return BASE_SIZE >> mip; }
	int MipOffset(int mip) const;

	// 所有 mip 级别的扁平存储。布局: mip0 (2048*2048), mip1 (1024*1024), ...
	std::vector<uint16_t> m_data;
};
