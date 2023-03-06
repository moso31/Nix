#include "NXGUIContentExplorer.h"
#include "NXScene.h"

NXGUIContentExplorer::NXGUIContentExplorer() :
    m_contentFilePath("D:\\NixAssets"),
    m_contentListTreeNodeFlags(ImGuiTreeNodeFlags_OpenOnDoubleClick)
{
}

void NXGUIContentExplorer::Render()
{
	ImGui::Begin("Content Explorer", 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::BeginTable("##table_content_explorer", 2, ImGuiTableFlags_Resizable, ImVec2(0, 0), 0.0f))
    {
        ImGui::TableSetupColumn("##content_list", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("##content_preview", ImGuiTableColumnFlags_NoHide);

		float fRowMinHeight = 0.0f;
		ImGui::TableNextRow(ImGuiTableRowFlags_None, fRowMinHeight);
		if (ImGui::TableSetColumnIndex(0))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            if (ImGui::BeginChild("##content_list_div", ImVec2(0, 0), true, ImGuiWindowFlags_None))
            {
                std::string strAssets = "Assets(" + m_contentFilePath.u8string() + ")";
                if (ImGui::TreeNode(strAssets.c_str()))
                {
                    RenderContentListFolder(m_contentFilePath, strAssets);
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
		}

		if (ImGui::TableSetColumnIndex(1))
		{
			static float fElementSize = 120.0f;
			ImGui::PushItemWidth(200.0f);
			ImGui::SliderFloat("##", &fElementSize, 30.0f, 120.0f, "Icon size");

			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            if (ImGui::BeginChild("##content_preview_div", ImVec2(0, 0), true, ImGuiWindowFlags_None))
            {
                float fAllElementsWidth = ImGui::GetColumnWidth();
                int iColumns = max(fAllElementsWidth / fElementSize, 1);
                int fActualSize = fAllElementsWidth / (float)iColumns;
                if (ImGui::BeginTable("##content_preview_table", iColumns, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoBordersInBody))
                {
                    for (int i = 0; i < 100; i++)
                    {
                        char buf[32];
                        sprintf_s(buf, "%03d", i);
                        ImGui::TableNextColumn();
                        ImGui::Button(buf, ImVec2(fActualSize, fActualSize));

                        auto textStr = "TestFileName";
                        float textWidth = ImGui::CalcTextSize(textStr).x;
                        float posX = ImGui::GetCursorPosX();
                        float textOffset = max(0.0f, (fActualSize - textWidth) * 0.5f);
                        ImGui::SetCursorPosX(posX + textOffset);
                        ImGui::Text(textStr);
                    }
                    ImGui::EndTable();
                }
            }
			ImGui::EndChild();
            ImGui::PopStyleVar();
		}

		ImGui::EndTable();
    }

	ImGui::End();
}

void NXGUIContentExplorer::RenderContentListFolder(const std::filesystem::path& FolderPath, const std::string& strForceName)
{
    for (auto const& p : std::filesystem::directory_iterator(FolderPath))
    {
        const std::string strFileName = p.path().stem().u8string();
        if (p.is_directory())
        {
            if (ImGui::TreeNode(strFileName.c_str()))
            {
                RenderContentListFolder(p.path(), "");
                ImGui::TreePop();
            }
        }
    }
}
