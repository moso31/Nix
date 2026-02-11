#include "NXVTIndirectTextureTracker.h"
#include "NXVirtualTexture.h" // CBufferRemoveSector, CBufferMigrateSector, VTLog
#include <algorithm>

NXVTIndirectTextureTracker::NXVTIndirectTextureTracker()
{
	// 预计算所有 mip 的总元素数: sum of (size>>mip)^2, mip 0..10
	int total = 0;
	for (int m = 0; m < MIP_LEVELS; m++)
	{
		int s = MipSize(m);
		total += s * s;
	}
	m_data.assign(total, EMPTY);
}

int NXVTIndirectTextureTracker::MipOffset(int mip) const
{
	int offset = 0;
	for (int m = 0; m < mip; m++)
	{
		int s = MipSize(m);
		offset += s * s;
	}
	return offset;
}

uint16_t NXVTIndirectTextureTracker::GetPixel(int mip, int x, int y) const
{
	if (mip < 0 || mip >= MIP_LEVELS) return EMPTY;
	int s = MipSize(mip);
	if (x < 0 || x >= s || y < 0 || y >= s) return EMPTY;
	return m_data[MipOffset(mip) + y * s + x];
}

// ---- 清除 ----

void NXVTIndirectTextureTracker::Clear()
{
	std::fill(m_data.begin(), m_data.end(), EMPTY);
	VTLog("[Tracker] Clear: all mips set to 0xFFFF\n");
}

// ---- 模拟删除 ----
// 对应 VTUpdateIndirectTextureBlock.fx 中的 CS_Remove

void NXVTIndirectTextureTracker::SimulateRemove(const CBufferRemoveSector& removeData)
{
	int mip = 0;
	Int2 base = removeData.imagePos * removeData.imageSize;

	for (int size = removeData.imageSize; size > 0; size >>= 1)
	{
		if (mip >= removeData.maxRemoveMip)
			break;

		int mipSz = MipSize(mip);
		int offset = MipOffset(mip);

		for (int cy = 0; cy < size; cy++)
		{
			for (int cx = 0; cx < size; cx++)
			{
				int px = (base.x >> mip) + cx;
				int py = (base.y >> mip) + cy;
				if (px < 0 || px >= mipSz || py < 0 || py >= mipSz)
					continue;

				uint16_t oldVal = m_data[offset + py * mipSz + px];
				if (oldVal != EMPTY)
				{
					m_data[offset + py * mipSz + px] = EMPTY;
					VTLog("[Tracker] Remove: mip%d (%d, %d) : %u -> EMPTY\n", mip, px, py, (unsigned)oldVal);
				}
			}
		}
		mip++;
	}
}

// ---- 模拟迁移 ----
// 对应 VTUpdateIndirectTextureBlock.fx 中的 CS_Migrate

void NXVTIndirectTextureTracker::SimulateMigrate(const CBufferMigrateSector& migrateData)
{
	Int2 fromBase = migrateData.fromImagePos * migrateData.fromImageSize;
	Int2 toBase = migrateData.toImagePos * migrateData.toImageSize;

	if (migrateData.fromImageSize < migrateData.toImageSize)
	{
		// 升采样
		int fromMip = 0;
		int toMip = migrateData.mipDelta;
		for (int size = migrateData.fromImageSize; size > 0; size >>= 1)
		{
			if (fromMip >= MIP_LEVELS || toMip >= MIP_LEVELS)
				break;

			int fromMipSz = MipSize(fromMip);
			int toMipSz = MipSize(toMip);
			int fromOff = MipOffset(fromMip);
			int toOff = MipOffset(toMip);

			for (int cy = 0; cy < size; cy++)
			{
				for (int cx = 0; cx < size; cx++)
				{
					int fpx = (fromBase.x >> fromMip) + cx;
					int fpy = (fromBase.y >> fromMip) + cy;
					int tpx = (toBase.x >> toMip) + cx;
					int tpy = (toBase.y >> toMip) + cy;

					if (fpx < 0 || fpx >= fromMipSz || fpy < 0 || fpy >= fromMipSz) continue;
					if (tpx < 0 || tpx >= toMipSz || tpy < 0 || tpy >= toMipSz) continue;

					uint16_t val = m_data[fromOff + fpy * fromMipSz + fpx];
					uint16_t oldTo = m_data[toOff + tpy * toMipSz + tpx];

					m_data[toOff + tpy * toMipSz + tpx] = val;
					m_data[fromOff + fpy * fromMipSz + fpx] = EMPTY;

					if (val != EMPTY || oldTo != EMPTY)
					{
						VTLog("[Tracker] Migrate(up): fromMip%d (%d,%d)->toMip%d (%d,%d) : val=%u, oldTo=%u\n",
							fromMip, fpx, fpy, toMip, tpx, tpy, (unsigned)val, (unsigned)oldTo);
					}
				}
			}
			fromMip++;
			toMip++;
		}
	}
	else
	{
		// 降采样
		int fromMip = migrateData.mipDelta;
		int toMip = 0;
		for (int size = migrateData.toImageSize; size > 0; size >>= 1)
		{
			if (fromMip >= MIP_LEVELS || toMip >= MIP_LEVELS)
				break;

			int fromMipSz = MipSize(fromMip);
			int toMipSz = MipSize(toMip);
			int fromOff = MipOffset(fromMip);
			int toOff = MipOffset(toMip);

			for (int cy = 0; cy < size; cy++)
			{
				for (int cx = 0; cx < size; cx++)
				{
					int fpx = (fromBase.x >> fromMip) + cx;
					int fpy = (fromBase.y >> fromMip) + cy;
					int tpx = (toBase.x >> toMip) + cx;
					int tpy = (toBase.y >> toMip) + cy;

					if (fpx < 0 || fpx >= fromMipSz || fpy < 0 || fpy >= fromMipSz) continue;
					if (tpx < 0 || tpx >= toMipSz || tpy < 0 || tpy >= toMipSz) continue;

					uint16_t val = m_data[fromOff + fpy * fromMipSz + fpx];
					uint16_t oldTo = m_data[toOff + tpy * toMipSz + tpx];

					m_data[toOff + tpy * toMipSz + tpx] = val;
					m_data[fromOff + fpy * fromMipSz + fpx] = EMPTY;

					if (val != EMPTY || oldTo != EMPTY)
					{
						VTLog("[Tracker] Migrate(down): fromMip%d (%d,%d)->toMip%d (%d,%d) : val=%u, oldTo=%u\n",
							fromMip, fpx, fpy, toMip, tpx, tpy, (unsigned)val, (unsigned)oldTo);
					}
				}
			}
			fromMip++;
			toMip++;
		}
	}
}

// ---- 模拟更新索引 ----
// 对应 UpdateIndirectTexture CS：在指定 gpuMip 的 (pageID.x, pageID.y) 处写入 physPageIndex。

void NXVTIndirectTextureTracker::SimulateUpdateIndex(int physPageIndex, const Int2& pageID, int gpuMip)
{
	if (gpuMip < 0 || gpuMip >= MIP_LEVELS)
		return;

	int s = MipSize(gpuMip);
	if (pageID.x < 0 || pageID.x >= s || pageID.y < 0 || pageID.y >= s)
		return;

	int offset = MipOffset(gpuMip);
	int idx = offset + pageID.y * s + pageID.x;
	uint16_t oldVal = m_data[idx];

	if (physPageIndex == -1)
	{
		m_data[idx] = EMPTY;
		if (oldVal != EMPTY)
			VTLog("[Tracker] UpdateIndex: mip%d (%d, %d) : %u -> EMPTY\n", gpuMip, pageID.x, pageID.y, (unsigned)oldVal);
	}
	else
	{
		uint16_t newVal = (uint16_t)physPageIndex;
		m_data[idx] = newVal;
		if (oldVal != newVal)
		{
			if (oldVal == EMPTY)
				VTLog("[Tracker] UpdateIndex: mip%d (%d, %d) : EMPTY -> %u\n", gpuMip, pageID.x, pageID.y, (unsigned)newVal);
			else
				VTLog("[Tracker] UpdateIndex: mip%d (%d, %d) : %u -> %u\n", gpuMip, pageID.x, pageID.y, (unsigned)oldVal, (unsigned)newVal);
		}
	}
}
