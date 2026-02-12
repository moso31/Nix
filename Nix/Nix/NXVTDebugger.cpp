#include "NXVTDebugger.h"

#ifdef _DEBUG

#include "NXVirtualTexture.h" // CBufferRemoveSector, CBufferMigrateSector
#include <algorithm>

// ---- 单例 ----

NXVTDebugger& NXVTDebugger::GetInstance()
{
	static NXVTDebugger instance;
	return instance;
}

NXVTDebugger::NXVTDebugger()
{
	// 初始化 IndirectTexture tracker 数据
	int total = 0;
	for (int m = 0; m < IT_MIP_LEVELS; m++)
	{
		int s = TrackerMipSize(m);
		total += s * s;
	}
	m_trackerData.assign(total, IT_EMPTY);
}

// ---- 日志 ----

void NXVTDebugger::Log(NXVTDebugBits bits, const char* fmt, ...)
{
	if (!IsEnabled(bits))
		return;

	va_list args;
	va_start(args, fmt);
	LogV(fmt, args);
	va_end(args);
}

void NXVTDebugger::LogV(const char* fmt, va_list args)
{
	if (m_output == NXVTDebugOutput::Console)
	{
		vprintf(fmt, args);
	}
	else
	{
		FILE* fp = nullptr;
		fopen_s(&fp, m_logFilePath, "a");
		if (fp)
		{
			vfprintf(fp, fmt, args);
			fclose(fp);
		}
	}
}

void NXVTDebugger::SetLogFilePath(const char* path)
{
	strncpy_s(m_logFilePath, path, _countof(m_logFilePath) - 1);
}

void NXVTDebugger::ClearLogFile()
{
	FILE* fp = nullptr;
	fopen_s(&fp, m_logFilePath, "w");
	if (fp)
		fclose(fp);
}

// ======================================================
// IndirectTexture CPU-side 模拟 (原 NXVTIndirectTextureTracker)
// ======================================================

int NXVTDebugger::TrackerMipOffset(int mip) const
{
	int offset = 0;
	for (int m = 0; m < mip; m++)
	{
		int s = TrackerMipSize(m);
		offset += s * s;
	}
	return offset;
}

uint16_t NXVTDebugger::TrackerGetPixel(int mip, int x, int y) const
{
	if (mip < 0 || mip >= IT_MIP_LEVELS) return IT_EMPTY;
	int s = TrackerMipSize(mip);
	if (x < 0 || x >= s || y < 0 || y >= s) return IT_EMPTY;
	return m_trackerData[TrackerMipOffset(mip) + y * s + x];
}

void NXVTDebugger::TrackerClear()
{
	std::fill(m_trackerData.begin(), m_trackerData.end(), IT_EMPTY);
	Log(VTDBG_TrackerClear, "[Tracker] Clear: all mips set to 0xFFFF\n");
}

void NXVTDebugger::TrackerSimulateRemove(const CBufferRemoveSector& removeData)
{
	int mip = 0;
	Int2 base = removeData.imagePos * removeData.imageSize;

	for (int size = removeData.imageSize; size > 0; size >>= 1)
	{
		if (mip >= removeData.maxRemoveMip)
			break;

		int mipSz = TrackerMipSize(mip);
		int offset = TrackerMipOffset(mip);

		for (int cy = 0; cy < size; cy++)
		{
			for (int cx = 0; cx < size; cx++)
			{
				int px = (base.x >> mip) + cx;
				int py = (base.y >> mip) + cy;
				if (px < 0 || px >= mipSz || py < 0 || py >= mipSz)
					continue;

				uint16_t oldVal = m_trackerData[offset + py * mipSz + px];
				if (oldVal != IT_EMPTY)
				{
					m_trackerData[offset + py * mipSz + px] = IT_EMPTY;
					Log(VTDBG_TrackerRemove, "[Tracker] Remove: mip%d (%d, %d) : %u -> EMPTY\n", mip, px, py, (unsigned)oldVal);
				}
			}
		}
		mip++;
	}
}

void NXVTDebugger::TrackerSimulateMigrate(const CBufferMigrateSector& migrateData)
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
			if (fromMip >= IT_MIP_LEVELS || toMip >= IT_MIP_LEVELS)
				break;

			int fromMipSz = TrackerMipSize(fromMip);
			int toMipSz = TrackerMipSize(toMip);
			int fromOff = TrackerMipOffset(fromMip);
			int toOff = TrackerMipOffset(toMip);

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

					uint16_t val = m_trackerData[fromOff + fpy * fromMipSz + fpx];
					uint16_t oldTo = m_trackerData[toOff + tpy * toMipSz + tpx];

					m_trackerData[toOff + tpy * toMipSz + tpx] = val;
					m_trackerData[fromOff + fpy * fromMipSz + fpx] = IT_EMPTY;

					if (val != IT_EMPTY || oldTo != IT_EMPTY)
					{
						Log(VTDBG_TrackerMigrate, "[Tracker] Migrate(up): fromMip%d (%d,%d)->toMip%d (%d,%d) : val=%u, oldTo=%u\n",
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
			if (fromMip >= IT_MIP_LEVELS || toMip >= IT_MIP_LEVELS)
				break;

			int fromMipSz = TrackerMipSize(fromMip);
			int toMipSz = TrackerMipSize(toMip);
			int fromOff = TrackerMipOffset(fromMip);
			int toOff = TrackerMipOffset(toMip);

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

					uint16_t val = m_trackerData[fromOff + fpy * fromMipSz + fpx];
					uint16_t oldTo = m_trackerData[toOff + tpy * toMipSz + tpx];

					m_trackerData[toOff + tpy * toMipSz + tpx] = val;
					m_trackerData[fromOff + fpy * fromMipSz + fpx] = IT_EMPTY;

					if (val != IT_EMPTY || oldTo != IT_EMPTY)
					{
						Log(VTDBG_TrackerMigrate, "[Tracker] Migrate(down): fromMip%d (%d,%d)->toMip%d (%d,%d) : val=%u, oldTo=%u\n",
							fromMip, fpx, fpy, toMip, tpx, tpy, (unsigned)val, (unsigned)oldTo);
					}
				}
			}
			fromMip++;
			toMip++;
		}
	}
}

void NXVTDebugger::TrackerSimulateUpdateIndex(int physPageIndex, const Int2& pageID, int gpuMip)
{
	if (gpuMip < 0 || gpuMip >= IT_MIP_LEVELS)
		return;

	int s = TrackerMipSize(gpuMip);
	if (pageID.x < 0 || pageID.x >= s || pageID.y < 0 || pageID.y >= s)
		return;

	int offset = TrackerMipOffset(gpuMip);
	int idx = offset + pageID.y * s + pageID.x;
	uint16_t oldVal = m_trackerData[idx];

	if (physPageIndex == -1)
	{
		m_trackerData[idx] = IT_EMPTY;
		if (oldVal != IT_EMPTY)
			Log(VTDBG_TrackerUpdateIndex, "[Tracker] UpdateIndex: mip%d (%d, %d) : %u -> EMPTY\n", gpuMip, pageID.x, pageID.y, (unsigned)oldVal);
	}
	else
	{
		uint16_t newVal = (uint16_t)physPageIndex;
		m_trackerData[idx] = newVal;
		if (oldVal != newVal)
		{
			if (oldVal == IT_EMPTY)
				Log(VTDBG_TrackerUpdateIndex, "[Tracker] UpdateIndex: mip%d (%d, %d) : EMPTY -> %u\n", gpuMip, pageID.x, pageID.y, (unsigned)newVal);
			else
				Log(VTDBG_TrackerUpdateIndex, "[Tracker] UpdateIndex: mip%d (%d, %d) : %u -> %u\n", gpuMip, pageID.x, pageID.y, (unsigned)oldVal, (unsigned)newVal);
		}
	}
}

#endif // _DEBUG