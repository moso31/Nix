#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct NXGPUProfileResult;

class NXGPUProfiler;
class NXGUIGPUProfiler
{
public:
	NXGUIGPUProfiler();
	virtual ~NXGUIGPUProfiler() {}

	void Render();

	void SetVisible(bool visible);
	bool IsVisible() const { return m_bShowWindow; }

private:
	void UpdateKnownPasses(const std::vector<NXGPUProfileResult>& results);

	bool m_bShowWindow = false;

	// 详细性能数据
	bool m_bShowDetailedStats = false;
	float m_detailedStatsDuration = 5.0f; // 统计最近多少秒的数据

	// Pass 过滤
	std::vector<std::string> m_allKnownPasses; // 所有已知的 pass 名称（有序）
	std::unordered_set<std::string> m_knownPassSet; // 快速查找
	std::unordered_map<std::string, bool> m_passVisibility; // true = 显示
	static constexpr int MAX_KNOWN_PASSES = 100;
};
