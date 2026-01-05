#include "NXGUITerrainStreamingDebug.h"
#include "Renderer.h"
#include "NXTerrainLODStreamer.h"
#include "NXTerrainLODStreamConfigs.h"

NXGUITerrainStreamingDebug::NXGUITerrainStreamingDebug(Renderer* pRenderer) : 
	m_pRenderer(pRenderer)
{
}

NXGUITerrainStreamingDebug::~NXGUITerrainStreamingDebug()
{
}

void NXGUITerrainStreamingDebug::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("地形流式加载调试"), &m_bVisible))
	{
		if (m_pRenderer)
		{
			auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
			if (pStreamer)
			{
				ImGui::Text(ImUtf8("地形流式加载调试选项"));
				ImGui::Separator();
				ImGui::Spacing();

				// 暂停异步加载选项
				ImGui::Checkbox(ImUtf8("暂停异步加载"), &g_terrainStreamDebug.bPauseAsyncLoading);
				
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// 显示当前状态
				if (g_terrainStreamDebug.bPauseAsyncLoading)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("状态: 已暂停"));
				}
				else
				{
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImUtf8("状态: 正常运行"));
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("TerrainLODStreamer 未初始化"));
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImUtf8("渲染器未设置"));
		}
	}
	ImGui::End();
}
