#include <algorithm>

#include "NXGUIContentExplorer.h"
#include "NXGUITexture.h"
#include "NXConverter.h"
#include "NXGUICommon.h"
#include "NXGUI.h"
#include "NXScene.h"

NXGUIContentExplorer::NXGUIContentExplorer(NXScene* pScene, NXGUITexture* pGUITexture) :
    m_pCurrentScene(pScene),
    m_pGUITexture(pGUITexture),
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
			static float fElementSize = 90.0f;
			ImGui::PushItemWidth(200.0f);
			ImGui::SliderFloat("##content_preview_slider_iconsize", &fElementSize, 30.0f, 120.0f, "Icon size");
            ImGui::SameLine();

            std::filesystem::path singleSelectFolderPath;
            int nSelect = 0;
            for (auto const& [_, elem] : m_selectionInfo) 
            {
                if (elem.bSelectedMask)
                {
                    if (!nSelect) singleSelectFolderPath = elem.filePath; // ��¼��ѡFolder·��
                    nSelect++;
                }
            }

            // ֻ�е�ѡ���ܽ������Ӳ���
            if (nSelect != 1)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            }

            if (ImGui::Button("add...##content_preview_add"))
                ImGui::OpenPopup("##content_preview_add_popup");

            if (nSelect != 1)
            {
                ImGui::PopStyleColor();
                ImGui::PopItemFlag();
            }

            if (ImGui::BeginPopup("##content_preview_add_popup"))
            {
                if (ImGui::Selectable("Material", false))
                {
                    GenerateMaterialResourceFile(singleSelectFolderPath);
                }
                ImGui::EndPopup();
            }

			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            if (ImGui::BeginChild("##content_preview_div", ImVec2(0, 0), true, ImGuiWindowFlags_None))
            {
                float fAllElementsWidth = ImGui::GetColumnWidth();
                int iColumns = max((int)(fAllElementsWidth / fElementSize), 1);
                float fActualSize = fAllElementsWidth / (float)iColumns;
                if (ImGui::BeginTable("##content_preview_table", iColumns, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoBordersInBody))
                {
                    for (auto const& [_, elem] : m_selectionInfo)
                    {
                        // �����������νṹ��ѡ�е�Folder...
                        if (elem.bSelectedMask)
                        {
                            // ...�µ��������ļ���
                            for (auto const& subElem : std::filesystem::directory_iterator(elem.filePath))
                            {
                                std::string strExtension = "";
                                std::string strExtensionText = "[unknown]";
                                if (subElem.is_directory())
                                    continue; // strTypeText = "folder"; // 2023.3.8 �ݲ�֧���ļ��У����þ�����
                                else if (subElem.path().has_extension())
                                {
                                    // ��ȡ��չ����ת����Сд
                                    strExtension = subElem.path().extension().string().c_str();
                                    strExtension = NXConvert::s2lower(strExtension);

                                    // *.nxInfo �����������ļ���������
                                    if (strExtension == ".nxinfo")
                                        continue;

                                    strExtensionText = strExtension;
                                }

                                ImGui::TableNextColumn();

                                // �ļ���/ͼ�갴ť����
                                if (ImGui::Button((strExtensionText + "##" + subElem.path().string()).c_str(), ImVec2(fActualSize, fActualSize)))
                                {
                                    //printf("%s\n", strExtensionText.c_str());
                                }

                                // �ļ���/ͼ�갴ť �϶��¼�
                                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                                {
                                    m_btnDrugData.srcPath = subElem.path();

                                    ImGui::SetDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING", &m_btnDrugData, sizeof(NXGUIContentExplorerButtonDrugData));
                                    ImGui::Text("(o_o)...");
                                    ImGui::EndDragDropSource();
                                }

                                // �ļ���/ͼ�갴ť �����¼�
                                if (ImGui::IsItemClicked() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                                {
                                    if (subElem.is_directory()) {} 
                                    else if (subElem.path().has_extension())
                                    {
                                        // �����ѡ��ͼƬ������Ҫ NXGUITexture ��ʾ��ͼ�������Ϣ
                                        if (NXConvert::IsImageFileExtension(subElem.path().extension().string()))
                                        {
                                            m_pGUITexture->SetImage(subElem.path());
                                        }
                                    }
                                }

                                //// �ļ���/ͼ�갴ť ˫���¼�
                                //if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                //{
                                //    if (subElem.is_directory()) {} // 2023.3.8 �ݲ�֧���ļ��У����þ�����
                                //    else if (subElem.path().has_extension())
                                //    {
                                //        if (strExtension == ".fbx")
                                //        {
                                //            // 2022.3.8 TODO: ˫��.fbx btnʱ�򳡾�����ģ��
                                //        }
                                //    }
                                //}

                                // �ļ���
                                std::string subElemFileName = subElem.path().stem().string().c_str();
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
    const std::string strFolderName = folderPath.stem().string();
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

void NXGUIContentExplorer::GenerateMaterialResourceFile(const std::filesystem::path& FolderPath)
{
    // �ж�һ�µ�ǰFolder�µ�����nmat�ļ��������ͷ�� "NewMat " + [��������] ����ʽ���ͽ����ּ�¼����
    // ���û�������Ĳ��ʣ��ͼ���Ϊ0
    std::string strJudge = "NewMat ";
    size_t nOffset = strJudge.length();
    int nMaxValue = 0;
    for (auto const& p : std::filesystem::directory_iterator(FolderPath))
        if (p.path().extension().string() == ".nmat")
        {
            std::string strStem = p.path().stem().string();
            if (strStem.length() <= nOffset)
                continue;

            std::string strFirst7 = strStem.substr(0, nOffset);
            std::string strLast = strStem.substr(nOffset, strStem.length() - nOffset);
            if (strFirst7 == strJudge)
            {
                if (std::all_of(strLast.begin(), strLast.end(), ::isdigit))
                nMaxValue = max(nMaxValue, std::stoi(strLast));
            }
        }

    // ������ɵ������� "NewMat " + [�������� + 1]��ȷ����������һ�������ظ���
    std::string strNewName = "NewMat " + std::to_string(nMaxValue + 1);
    std::string strNewPath = strNewName + ".nmat";
    std::filesystem::path newPath = FolderPath / strNewPath;

    // Ĭ���½�һ��StandardPBR����
    NXGUICommon::CreateDefaultMaterialFile(newPath, strNewName);
}