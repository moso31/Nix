#include "BaseDefs/DearImGui.h"
#include "NXGUIGPUProfiler.h"
#include "NXGPUProfiler.h"

NXGUIGPUProfiler::NXGUIGPUProfiler()
{
}

void NXGUIGPUProfiler::SetVisible(bool visible)
{
	m_bShowWindow = visible;
	if (g_pGPUProfiler)
		g_pGPUProfiler->SetEnabled(visible);
}

void NXGUIGPUProfiler::UpdateKnownPasses(const std::vector<NXGPUProfileResult>& results)
{
	for (const auto& result : results)
	{
		if (m_knownPassSet.find(result.passName) == m_knownPassSet.end())
		{
			if ((int)m_allKnownPasses.size() < MAX_KNOWN_PASSES)
			{
				m_allKnownPasses.push_back(result.passName);
				m_knownPassSet.insert(result.passName);
				m_passVisibility[result.passName] = true; // 默认可见
			}
		}
	}
}

void NXGUIGPUProfiler::Render()
{
	if (!m_bShowWindow)
		return;

	if (!g_pGPUProfiler)
		return;

	ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_FirstUseEver);
	bool bWindowOpen = true;
	if (ImGui::Begin("GPU Profiler", &bWindowOpen))
	{
		if (!g_pGPUProfiler->IsEnabled())
			g_pGPUProfiler->SetEnabled(true); // 窗口打开时确保启用

		const auto& results = g_pGPUProfiler->GetLastFrameResults();

		// 更新已知 pass 列表
		UpdateKnownPasses(results);

        ImGuiIO& io = ImGui::GetIO();
        double gpuTotalMs = g_pGPUProfiler->GetLastFrameTotalTimeMs();
        double frameTimeMs = 1000.0 / io.Framerate;

		// 计算启用和未启用的耗时
		double enabledTotalMs = 0.0;
		double disabledTotalMs = 0.0;
		for (const auto& result : results)
		{
			auto it = m_passVisibility.find(result.passName);
			bool visible = (it == m_passVisibility.end()) || it->second;
			if (visible)
				enabledTotalMs += result.timeMs;
			else
				disabledTotalMs += result.timeMs;
		}

        ImGui::Separator();

        // FPS / 帧时间
        ImGui::Text(ImUtf8("帧数 (FPS): %.1f"), io.Framerate);
        ImGui::SameLine();
        ImGui::Text(ImUtf8("帧时间 (1/FPS): %.3f ms"), frameTimeMs);

		// GPU 总耗时（拆分为两个数据）
        ImGui::Text(ImUtf8("GPU 总耗时: %.3f ms"), gpuTotalMs);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), ImUtf8("已启用: %.3f ms"), enabledTotalMs);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ImUtf8("未启用: %.3f ms"), disabledTotalMs);

		// 未统计时间
        double unmeasuredMs = frameTimeMs - gpuTotalMs;
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ImUtf8("未统计时间: %.3f ms"), unmeasuredMs);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), ImUtf8("(CPU工作, Present, 垂直同步, 多视口)"));

        ImGui::Separator();

		// 工具栏：Pass 过滤按钮 + 详细性能数据选项
		if (ImGui::Button(ImUtf8("Pass 过滤")))
		{
			ImGui::OpenPopup("PassFilterPopup");
		}
		ImGui::SameLine();
		ImGui::Checkbox(ImUtf8("详细性能数据"), &m_bShowDetailedStats);
		if (m_bShowDetailedStats)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			ImGui::SliderFloat(ImUtf8("秒"), &m_detailedStatsDuration, 1.0f, 30.0f, "%.0f");
			g_pGPUProfiler->SetHistoryDuration(m_detailedStatsDuration);
		}

		// Pass 过滤弹窗
		if (ImGui::BeginPopup("PassFilterPopup"))
		{
			ImGui::Text(ImUtf8("选择要显示的 Pass（共 %d 个）:"), (int)m_allKnownPasses.size());
			ImGui::Separator();

			// 全选/取消全选
			if (ImGui::Button(ImUtf8("全选")))
			{
				for (auto& [name, visible] : m_passVisibility)
					visible = true;
			}
			ImGui::SameLine();
			if (ImGui::Button(ImUtf8("全不选")))
			{
				for (auto& [name, visible] : m_passVisibility)
					visible = false;
			}
			ImGui::Separator();

			// 可滚动的 checkbox 列表
			float listHeight = std::min(400.0f, (float)m_allKnownPasses.size() * 25.0f + 10.0f);
			if (ImGui::BeginChild("PassFilterList", ImVec2(300, listHeight), ImGuiChildFlags_Border))
			{
				for (const auto& passName : m_allKnownPasses)
				{
					bool visible = m_passVisibility[passName];
					if (ImGui::Checkbox(passName.c_str(), &visible))
					{
						m_passVisibility[passName] = visible;
					}
				}
			}
			ImGui::EndChild();
			ImGui::EndPopup();
		}

		ImGui::Separator();

		// 表格
		int columnCount = m_bShowDetailedStats ? 6 : 3;
        float availableHeight = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginTable("PassTimings", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, availableHeight)))
        {
            ImGui::TableSetupColumn(ImUtf8("Pass 名称"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(ImUtf8("耗时 (ms)"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
			if (m_bShowDetailedStats)
			{
				ImGui::TableSetupColumn(ImUtf8("最小 (ms)"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableSetupColumn(ImUtf8("最大 (ms)"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableSetupColumn(ImUtf8("平均 (ms)"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
			}
            ImGui::TableSetupColumn(ImUtf8("占比"), ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();

            for (const auto& result : results)
            {
				// 检查 pass 是否启用显示
				auto it = m_passVisibility.find(result.passName);
				bool visible = (it == m_passVisibility.end()) || it->second;
				if (!visible)
					continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", result.passName.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", result.timeMs);

				if (m_bShowDetailedStats)
				{
					NXGPUProfileStats stats = g_pGPUProfiler->GetPassStats(result.passName, m_detailedStatsDuration);
					ImGui::TableNextColumn();
					if (stats.sampleCount > 0)
						ImGui::Text("%.3f", stats.minMs);
					else
						ImGui::Text("-");
					ImGui::TableNextColumn();
					if (stats.sampleCount > 0)
						ImGui::Text("%.3f", stats.maxMs);
					else
						ImGui::Text("-");
					ImGui::TableNextColumn();
					if (stats.sampleCount > 0)
						ImGui::Text("%.3f", stats.avgMs);
					else
						ImGui::Text("-");
				}

                ImGui::TableNextColumn();
                float percentage = (gpuTotalMs > 0.0) ? (float)(result.timeMs / gpuTotalMs * 100.0) : 0.0f;
                ImGui::Text("%.1f%%", percentage);
            }
            ImGui::EndTable();
        }
	}
	ImGui::End();

	// 窗口关闭时禁用profiler
	if (!bWindowOpen)
		SetVisible(false);
}
