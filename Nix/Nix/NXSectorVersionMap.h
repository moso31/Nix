#pragma once
#include "BaseDefs/Math.h"
#include <vector>

// 记录每个sector的版本号
// 每当流式加载更新一个区域的nodeID时，对应区域的所有Sector版本+1
// 版本号在VirtualTexture中使用，确保物理页能时刻维持最新状态。
// 按sector储存基本单位（默认16384^2/64=256^2）
class NXSectorVersionMap
{
public:
	NXSectorVersionMap() : m_size(0, 0) {}
	~NXSectorVersionMap() {}

	// 初始化版本图，设置大小并将所有版本号清零
	void Init(const Int2& size)
	{
		m_size = size;
		int totalSize = size.x * size.y;
		m_versions.resize(totalSize);
		memset(m_versions.data(), 0, totalSize * sizeof(uint32_t));
	}

	// 更新指定区域的版本号（递增）
	// pos: 起始sector坐标
	// size: 区域大小（size x size）
	void UpdateVersion(const Int2& pos, int size)
	{
		for (int y = pos.y; y < pos.y + size; ++y)
		{
			for (int x = pos.x; x < pos.x + size; ++x)
			{
				int index = y * m_size.x + x;
				m_versions[index]++;
			}
		}
	}

	// 获取指定sector的版本号
	uint32_t GetVersion(const Int2& pos) const
	{
		int index = pos.y * m_size.x + pos.x;
		return m_versions[index];
	}

	// 获取版本图大小
	const Int2& GetSize() const { return m_size; }

	// 打印版本号（按行列打印）
	void Print() const
	{
		printf("SectorVersionMap (%d x %d):\n", m_size.x, m_size.y);
		for (int y = 0; y < m_size.y; ++y)
		{
			for (int x = 0; x < m_size.x; ++x)
			{
				int index = y * m_size.x + x;
				printf("%3d ", m_versions[index]);
			}
			printf("\n");
		}
		printf("\n");
	}

private:
	Int2 m_size;						// 版本图的尺寸
	std::vector<uint32_t> m_versions;	// 版本号数组（1D存储）
};
