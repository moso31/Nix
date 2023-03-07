#include "NXGUIContentExplorer.h"
#include "NXScene.h"
#include <algorithm>

NXGUIContentExplorer::NXGUIContentExplorer() :
    m_contentFilePath("D:\\NixAssets")
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
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
            if (ImGui::BeginChild("##content_list_div", ImVec2(0, 0), true, ImGuiWindowFlags_None))
            {
                RenderContentFolder(m_contentFilePath);
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
                    for (auto const& [_, elem] : m_selectionInfo)
                    {
                        // 遍历所有树形结构中选中的Folder...
                        if (elem.bSelectedMask)
                        {
                            // ...下的所有子文件。
                            for (auto const& subElem : std::filesystem::directory_iterator(elem.filePath))
                            {
                                std::string strTypeText = "[unknown]";
                                if (subElem.is_directory()) 
                                    strTypeText = "folder";
                                else if (subElem.path().has_extension())
                                    strTypeText = subElem.path().extension().u8string().c_str();

                                // 字符串转小写
                                std::transform(strTypeText.begin(), strTypeText.end(), strTypeText.begin(), [](UCHAR c) { return std::tolower(c); }); 

                                ImGui::TableNextColumn();

                                // 文件夹/图标按钮本体
                                ImGui::Button(strTypeText.c_str(), ImVec2(fActualSize, fActualSize));

                                // 文件名
                                std::string subElemFileName = subElem.path().stem().u8string().c_str();
                                float textWidth = ImGui::CalcTextSize(subElemFileName.c_str()).x;
                                float posX = ImGui::GetCursorPosX();
                                float textOffset = max(0.0f, (fActualSize - textWidth) * 0.5f);
                                ImGui::SetCursorPosX(posX + textOffset);
                                ImGui::Text(subElemFileName.c_str());
                            }
                        }
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

void NXGUIContentExplorer::RenderContentFolder(const std::filesystem::path& folderPath)
{
    const std::string strFolderName = folderPath.stem().u8string();
    size_t nHashFilePath = std::filesystem::hash_value(folderPath);

    ImGuiTreeNodeFlags eTreeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
    if (m_selectionInfo[nHashFilePath].bSelectedMask)
        eTreeNodeFlags |= ImGuiTreeNodeFlags_Selected;

    bool bOpen = ImGui::TreeNodeEx(strFolderName.c_str(), eTreeNodeFlags);
    bool bClicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
    if (bOpen)
    {
        RenderContentFolderList(folderPath);
        ImGui::TreePop();
    }

    if (bClicked)
    {
        if (!ImGui::GetIO().KeyCtrl)
            for (auto& [_, elem] : m_selectionInfo) elem.bSelectedMask = false;

        m_selectionInfo[nHashFilePath].bSelectedMask = !m_selectionInfo[nHashFilePath].bSelectedMask;
        m_selectionInfo[nHashFilePath].filePath = folderPath;
    }
}

void NXGUIContentExplorer::RenderContentFolderList(const std::filesystem::path& FolderPath)
{
    for (auto const& p : std::filesystem::directory_iterator(FolderPath))
        if (p.is_directory()) RenderContentFolder(p.path());
}
