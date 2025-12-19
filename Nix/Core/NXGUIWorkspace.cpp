#include "BaseDefs/DearImGui.h"

#include "NXGUIWorkspace.h"
#include "NXGUI.h"
#include "NXGUIHoudiniTerrainExporter.h"
#include "NXGUITerrainMaterialGenerator.h"
#include "NXFileSystemHelper.h"

void NXGUIWorkspace::Init(NXGUI* pGUI)
{
#if DEBUG || _DEBUG
    m_strMode = "(Debug Mode)";
#else
    m_strMode = "(Release Mode)";
#endif

    using namespace std::chrono;
    year_month_day latestTime(year(2023), month(6), day(3));
    year_month_day lastFileModifiedTime = NXFilesystemHelper::GetLatestFileModifiedTime(".\\");
    const year_month_day& versionTime(lastFileModifiedTime > latestTime ? lastFileModifiedTime : latestTime);

    m_strVersion = "Nix Rendering Frame, version " + 
        std::to_string((int)versionTime.year()) + "." + std::to_string((unsigned)versionTime.month()) + "." + std::to_string((unsigned)versionTime.day());

    m_pGUI = pGUI;
}

void NXGUIWorkspace::Render()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    //if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 7.0f));
    ImGui::Begin("DockSpace Demo", &m_bOpen, window_flags);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    // Menu
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 7.0f));
    bool bMenuBar = ImGui::BeginMenuBar();
    ImGui::PopStyleVar();
    float fMenuBarWidth = ImGui::GetWindowWidth();
    if (bMenuBar)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(7.0f, 7.0f));
        bool bMenu = ImGui::BeginMenu("File");
        ImGui::PopStyleVar();
        if (bMenu)
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.

            if (ImGui::MenuItem("Menu")) {}
            if (ImGui::MenuItem("Bar")) {}
            if (ImGui::MenuItem("Is")) {}
            if (ImGui::MenuItem("Developing...(Alt + 1234)")) {}
            ImGui::Separator();

            if (ImGui::MenuItem("Exit", NULL, false)) {}
            ImGui::EndMenu();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(7.0f, 7.0f));
        bool bToolsMenu = ImGui::BeginMenu(ImUtf8("地形"));
        ImGui::PopStyleVar();
        if (bToolsMenu)
        {
            if (ImGui::MenuItem(ImUtf8("Houdini地形导出..."))) 
            {
                m_pGUI->GetGUIHoudiniTerrainExporter()->SetVisible(true);
            }
            if (ImGui::MenuItem(ImUtf8("地形材质纹理生成..."))) 
            {
                m_pGUI->GetGUITerrainMaterialGenerator()->SetVisible(true);
            }
            ImGui::EndMenu();
        }

        // 样式：右对齐
        ImGui::SetCursorPosX(fMenuBarWidth - ImGui::CalcTextSize(std::string(m_strVersion + m_strMode).c_str()).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

        // 样式：右移一小段
        //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 0.6f));
        ImGui::Text(m_strVersion.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 0.5f));
        ImGui::Text(m_strMode.c_str());
        ImGui::PopStyleColor();

        ImGui::EndMenuBar();
    }

    ImGui::End();
}
