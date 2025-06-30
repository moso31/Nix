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
// ÿ֡���ã��� s_terrain_system_dock_inited==false ʱ�Ż����¹��� DockLayout
void NXGUITerrainSystem::Render()
{
    if (!m_bShowWindow) return;

    // -------------------- 1. �������ڣ������һ�� DockSpace�� --------------------
    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking |            // ��������� DockSpace �Ͻ�ȥ
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Terrain System", &m_bShowWindow, host_flags);

    ImGuiID dockspace_id = ImGui::GetID("TerrainSystemDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0),
        ImGuiDockNodeFlags_PassthruCentralNode);  // ����������͸��ȥ

    // -------------------- 2. ��һ�� / ���� ʱ���� DockLayout --------------------
    if (!s_terrain_system_dock_inited)
    {
        s_terrain_system_dock_inited = true;

        // �Ѿɽڵ�ȫ��������� ini ����������Ĳ��ֻḲ�����½��ģ�
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id,
            ImGuiDockNodeFlags_DockSpace |
            ImGuiDockNodeFlags_NoSplit);  // ���ڵ��ֹ�ٴ� split
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

        // ���ҷָ0.25f = 25% ��ȸ���ࣩ
        ImGuiID dock_left, dock_right;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, &dock_left, &dock_right);

        // �Ҳ������·ָ0.25f = 25% �߶ȸ����£�
        ImGuiID dock_right_bot, dock_right_top;
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.25f, &dock_right_bot, &dock_right_top);

        // ---- ��ÿ���ӽڵ�ȥ��ǩ�������ܱ����ߣ��������Ϸָ��� ----
        ImGuiDockNode* node = nullptr;
        node = ImGui::DockBuilderGetNode(dock_left);         node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        node = ImGui::DockBuilderGetNode(dock_right_top);    node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        node = ImGui::DockBuilderGetNode(dock_right_bot);    node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

        // ---- �������Ӵ��� Dock ��ȥ ----
        ImGui::DockBuilderDockWindow("##TerrainList", dock_left);
        ImGui::DockBuilderDockWindow("##HeightMap", dock_right_top);
        ImGui::DockBuilderDockWindow("##TerrainButtons", dock_right_bot);

        ImGui::DockBuilderFinish(dockspace_id);  // ��ɣ�
    }
    ImGui::End(); // -------- host ������Ⱦ��ϣ�DockSpace ����Ч�� --------


    // -------------------- 3. �Ӵ������� --------------------
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
    ImGui::Dummy(ImVec2(0, 0)); // TODO: ��Ⱦ�߶�ͼ����
}

void NXGUITerrainSystem::Render_Tools()
{
    if (ImGui::Button("Generate")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Delete")) { /* ... */ }
    ImGui::SameLine();
    if (ImGui::Button("Export")) { /* ... */ }
}
