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

        ImGuiIO& io = ImGui::GetIO();
        double gpuTotalMs = g_pGPUProfiler->GetLastFrameTotalTimeMs();
        double frameTimeMs = 1000.0 / io.Framerate;

        ImGui::Separator();

        // 对比显示n
        ImGui::Text(ImUtf8("帧数 (FPS): %.1f"), io.Framerate);
        ImGui::SameLine();
        ImGui::Text(ImUtf8("帧时间 (1/FPS): %.3f ms"), frameTimeMs);

        ImGui::Text(ImUtf8("GPU 总耗时: %.3f ms"), gpuTotalMs);
        ImGui::SameLine();

        // 计算差值
        double unmeasuredMs = frameTimeMs - gpuTotalMs;
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ImUtf8("未统计时间: %.3f ms"), unmeasuredMs);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), ImUtf8("(CPU工作, Present, 垂直同步, 多视口)"));

        ImGui::Separator();

        const auto& results = g_pGPUProfiler->GetLastFrameResults();

        // 表格显示每个 Pass 的时间
        // 计算可用空间：窗口内容区域减去已使用的空间，预留一些边距
        float availableHeight = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginTable("PassTimings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, availableHeight)))
        {
            ImGui::TableSetupColumn(ImUtf8("Pass 名称"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(ImUtf8("耗时 (ms)"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn(ImUtf8("占比"), ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();

            for (const auto& result : results)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", result.passName.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", result.timeMs);
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
