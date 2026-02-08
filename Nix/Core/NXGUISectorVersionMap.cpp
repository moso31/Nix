#include "NXGUISectorVersionMap.h"
#include "Renderer.h"
#include "NXTerrainLODStreamer.h"

NXGUISectorVersionMap::NXGUISectorVersionMap(Renderer* pRenderer) :
	m_pRenderer(pRenderer)
{
}

NXGUISectorVersionMap::~NXGUISectorVersionMap()
{
}

void NXGUISectorVersionMap::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(600.0f, 650.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(ImUtf8("全局Sector版本号"), &m_bVisible))
	{
		if (!m_pRenderer)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImUtf8("渲染器未设置"));
			ImGui::End();
			return;
		}

		auto* pStreamer = m_pRenderer->GetTerrainLODStreamer();
		if (!pStreamer)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("TerrainLODStreamer 未初始化"));
			ImGui::End();
			return;
		}

		const NXSectorVersionMap& versionMap = pStreamer->GetSectorVersionMap();
		Int2 mapSize = versionMap.GetSize();

		ImGui::Text(ImUtf8("Sector版本图大小: %d x %d"), mapSize.x, mapSize.y);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);
		ImGui::SliderFloat(ImUtf8("缩放"), &m_cellSize, 4.0f, 40.0f, "%.0f px");
		ImGui::Separator();
		ImGui::Spacing();

		if (mapSize.x <= 0 || mapSize.y <= 0)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImUtf8("版本图未初始化"));
			ImGui::End();
			return;
		}

		// 显示版本号网格
		ImGui::BeginChild("VersionGrid", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		// 鼠标中键拖拽滚动
		if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
			ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
		}

		float cellSize = m_cellSize;
		float fontSize = ImGui::GetFontSize();
		bool bShowText = (cellSize >= fontSize + 4.0f); // 单元格够大时才显示文字

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 mousePos = ImGui::GetMousePos();

		for (int y = 0; y < mapSize.y; ++y)
		{
			for (int x = 0; x < mapSize.x; ++x)
			{
				uint32_t version = versionMap.GetVersion(Int2(x, y));

				ImVec2 cellMin = ImVec2(canvasPos.x + x * cellSize, canvasPos.y + y * cellSize);
				ImVec2 cellMax = ImVec2(cellMin.x + cellSize, cellMin.y + cellSize);

				// 根据版本号设置颜色
				ImU32 cellColor;
				if (version == 0)
				{
					cellColor = IM_COL32(40, 40, 40, 255);
				}
				else
				{
					float intensity = std::min(version / 10.0f, 1.0f);
					int colorValue = (int)(intensity * 200) + 55;
					cellColor = IM_COL32(55, 55, colorValue, 255);
				}

				drawList->AddRectFilled(cellMin, cellMax, cellColor);
				drawList->AddRect(cellMin, cellMax, IM_COL32(80, 80, 80, 255));

				if (bShowText)
				{
					// 单元格足够大：显示文字
					char text[32];
					sprintf_s(text, "%u", version);
					ImVec2 textSize = ImGui::CalcTextSize(text);
					ImVec2 textPos = ImVec2(
						cellMin.x + (cellSize - textSize.x) * 0.5f,
						cellMin.y + (cellSize - textSize.y) * 0.5f
					);

					ImU32 textColor = (version > 5) ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
					drawList->AddText(textPos, textColor, text);
				}
				else
				{
					// 单元格太小：鼠标悬停时显示tooltip
					if (mousePos.x >= cellMin.x && mousePos.x < cellMax.x &&
						mousePos.y >= cellMin.y && mousePos.y < cellMax.y)
					{
						// 高亮边框
						drawList->AddRect(cellMin, cellMax, IM_COL32(255, 255, 0, 255));

						ImGui::BeginTooltip();
						ImGui::Text("Sector (%d, %d): %u", x, y, version);
						ImGui::EndTooltip();
					}
				}
			}
		}

		ImGui::Dummy(ImVec2(mapSize.x * cellSize, mapSize.y * cellSize));
		ImGui::EndChild();
	}
	ImGui::End();
}
