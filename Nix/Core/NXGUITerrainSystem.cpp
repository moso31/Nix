#include "BaseDefs/DearImGui.h"
#include "NXGUITerrainSystem.h"
#include "NXScene.h"

static bool s_terrain_system_dock_inited = false;

NXGUITerrainSystem::NXGUITerrainSystem(NXScene* pScene /*=nullptr*/)
    : m_pCurrentScene(pScene)
    , m_bShowWindow(false)
{
}

// -----------------------------------------------------------------------------
// 每帧调用；当 s_terrain_system_dock_inited==false 时才会重新构建 DockLayout
void NXGUITerrainSystem::Render()
{
    if (!m_bShowWindow) return;

    // -------------------- 1. 宿主窗口（里面放一个 DockSpace） --------------------
    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking |            // 自身不被别的 DockSpace 拖进去
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Terrain System", &m_bShowWindow, host_flags);

    ImGuiID dockspace_id = ImGui::GetID("TerrainSystemDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0),
        ImGuiDockNodeFlags_PassthruCentralNode);  // 让主窗背景透过去

    // -------------------- 2. 第一次 / 重置 时构建 DockLayout --------------------
    if (!s_terrain_system_dock_inited)
    {
        s_terrain_system_dock_inited = true;

        // 把旧节点全清掉（否则 ini 里曾保存过的布局会覆盖你新建的）
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id,
            ImGuiDockNodeFlags_DockSpace |
            ImGuiDockNodeFlags_NoSplit);  // 根节点禁止再次 split
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

        // 左右分割（0.25f = 25% 宽度给左侧）
        ImGuiID dock_left, dock_right;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, &dock_left, &dock_right);

        // 右侧再上下分割（0.25f = 25% 高度给右下）
        ImGuiID dock_right_bot, dock_right_top;
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.25f, &dock_right_bot, &dock_right_top);

        // ---- 给每个子节点去标签栏：不能被拖走，但还能拖分隔线 ----
        ImGuiDockNode* node = nullptr;
        node = ImGui::DockBuilderGetNode(dock_left);         node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        node = ImGui::DockBuilderGetNode(dock_right_top);    node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        node = ImGui::DockBuilderGetNode(dock_right_bot);    node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

        // ---- 把三个子窗口 Dock 进去 ----
        ImGui::DockBuilderDockWindow("##TerrainList", dock_left);
        ImGui::DockBuilderDockWindow("##HeightMap", dock_right_top);
        ImGui::DockBuilderDockWindow("##TerrainButtons", dock_right_bot);

        ImGui::DockBuilderFinish(dockspace_id);  // 完成！
    }
    ImGui::End(); // -------- host 窗口渲染完毕（DockSpace 已生效） --------


    // -------------------- 3. 子窗口内容 --------------------
    ImGui::Begin("##TerrainList", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    {
        Render_List();
    }
    ImGui::End();

    ImGui::Begin("##HeightMap", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    {
        Render_Map();
    }
    ImGui::End();

    ImGui::Begin("##TerrainButtons", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    {
        Render_Tools();
    }
    ImGui::End();
}

void NXGUITerrainSystem::Render_List()
{
    ImGui::Text("Terrain List");
    ImGui::Separator();
    for (int i = 0; i < 10; ++i)
        ImGui::Selectable(("Terrain " + std::to_string(i)).c_str());
}

void NXGUITerrainSystem::Render_Map()
{
    ImGui::Text("Height Map");
    ImGui::Dummy(ImVec2(0, 0)); // TODO: 渲染高度图纹理
}

void NXGUITerrainSystem::Render_Tools()
{
    if (ImGui::Button("Generate")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Export")) { /* ... */ }
}
