#include "BaseDefs/DearImGui.h"
#include "NXGUITerrainSystem.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGUICommon.h"
#include "NXConverter.h"

static bool s_terrain_system_dock_inited = false;

NXGUITerrainSystem::NXGUITerrainSystem(NXScene* pScene /*=nullptr*/) : 
    m_pCurrentScene(pScene),
    m_pPickingTerrain(nullptr),
    m_bShowWindow(false),
    m_bPickTerrainSelectionChanged(false)
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
    if (m_pCurrentScene)
    {
        for (auto& [pTerrNodeId, pTerrain] : m_pCurrentScene->GetTerrains())
        {
            bool bSelected = (pTerrain == m_pPickingTerrain);

            std::string strNodeId = std::to_string(pTerrain->GetTerrainNode().x) + ", " + std::to_string(pTerrain->GetTerrainNode().y);
            std::string strName = pTerrain->GetName().c_str() + std::string("(id: ") + strNodeId + ")";
            if (ImGui::Selectable(strName.c_str(), bSelected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_pPickingTerrain = pTerrain;
                m_bPickTerrainSelectionChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();

                if (m_bPickTerrainSelectionChanged)
                {
                    ImGui::SetScrollHereY(0.5f);
                    m_bPickTerrainSelectionChanged = false;
                }
            }
        }
    }
}
void NXGUITerrainSystem::Render_Map()
{
    if (!m_pCurrentScene) return;

    const auto& terrains = m_pCurrentScene->GetTerrains();
    if (terrains.empty()) return;

    /* ---------- 1. �ȼ��� X/Y �����Сֵ ---------- */
    short minX = SHRT_MAX, minY = SHRT_MAX;
    short maxX = -SHRT_MAX, maxY = -SHRT_MAX;

    for (const auto& [nodeId, pTerrain] : terrains)
    {
        minX = std::min(minX, nodeId.x);
        minY = std::min(minY, nodeId.y);
        maxX = std::max(maxX, nodeId.x);
        maxY = std::max(maxY, nodeId.y);
    }

    const float kSpacing = 2.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kSpacing, kSpacing));

    const ImVec2 kBtnSize(50.0f, 50.0f);
    Ntr<NXTexture2D> pEmptyTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
    NXShVisDescHeap->PushFluid(pEmptyTex->GetSRV());
    auto& srvEmptyHandle = NXShVisDescHeap->Submit();

    for (short y = maxY; y >= minY; y--) // flip Y
    {
        for (short x = minX; x <= maxX; x++)
        {
            auto* pTerrain = m_pCurrentScene->GetTerrain(x, y);
            NXTerrainLayer* pTerrainLayer = nullptr;

            ImGui::PushID((y << 16) + x);

            // ���ز���ʾ�߶�ͼ��
            bool bHeightTexExist = false;
            if (pTerrain)
            {
                pTerrainLayer = pTerrain->GetTerrainLayer();
                if (pTerrainLayer)
                {
                    auto& pTerrLayerTex = pTerrainLayer->GetHeightMapTexture();
                    if (pTerrLayerTex.IsValid())
                    {
                        NXShVisDescHeap->PushFluid(pTerrLayerTex->GetSRV());
                        auto& srvHandle = NXShVisDescHeap->Submit();
                        if (ImGui::ImageButton("##TerrainBlock", (ImTextureID)srvHandle.ptr, kBtnSize))
                        {
                            m_pPickingTerrain = pTerrain;
                            m_bPickTerrainSelectionChanged = true;
                        }
                        bHeightTexExist = true;
                    }
                }
            }
            
            if (!bHeightTexExist) // ���û�и߶�ͼ���ؿ�����
            {
                if (ImGui::ImageButton("##TerrainBlock", (ImTextureID)srvEmptyHandle.ptr, kBtnSize))
                {
                    m_pPickingTerrain = pTerrain;
                    m_bPickTerrainSelectionChanged = true;
                }
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
                {
                    auto pDropData = static_cast<const NXGUIAssetDragData*>(payload->Data);
                    if (pDropData && pDropData->srcPath.has_extension() && NXConvert::IsRawFileExtension(pDropData->srcPath.extension().string()))
                    {
                        // ���������ļ��Ǹ߶�ͼ��Raw �ļ����������õ���Ӧ�� TerrainLayer ��
                        if (pTerrainLayer)
                        {
                            pTerrainLayer->SetHeightMapPath(pDropData->srcPath);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();

            if (x < maxX) ImGui::SameLine();
        }
    }

    ImGui::PopStyleVar();
}

void NXGUITerrainSystem::Render_Tools()
{
    if (ImGui::Button("Bake GPUTerrain data....")) 
    {
    }
}

void NXGUITerrainSystem::GenerateFile_Tex2DArray_HeightMap()
{
    if (m_pCurrentScene)
    {
        for (auto& [pTerrNodeId, pTerrain] : m_pCurrentScene->GetTerrains())
        {
            if (pTerrain)
            {
                auto* pTerrainLayer = pTerrain->GetTerrainLayer();
                if (pTerrainLayer)
                {
                    pTerrain->GetTerrainNode();
                    pTerrainLayer->GetHeightMapPath();
                }
            }
        }
    }
}
