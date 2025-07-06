#include "BaseDefs/DearImGui.h"
#include "NXGUITerrainSystem.h"
#include "NXScene.h"
#include "NXTerrain.h"
#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXTextureMaker.h"
#include "NXGPUTerrainManager.h"
#include "NXTerrainCommon.h"

static bool s_terrain_system_dock_inited = false;

NXGUITerrainSystem::NXGUITerrainSystem(NXScene* pScene /*=nullptr*/) :
    m_pCurrentScene(pScene),
    m_pPickingTerrain(nullptr),
    m_bShowWindow(false),
    m_bPickTerrainSelectionChanged(false)
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

    ProcessAsyncCallback();

    ImGui::End();
}

void NXGUITerrainSystem::ProcessAsyncCallback()
{
    const auto& terrains = m_pCurrentScene->GetTerrains();
    if (m_bNeedUpdateTerrainLayerFiles)
    {
        // 遍历关联的所有地形，并保存每个terrainLayers
        for (const auto& [nodeId, pTerrain] : terrains)
        {
            auto* pTerrainLayer = pTerrain->GetTerrainLayer();
            if (pTerrainLayer)
            {
                pTerrainLayer->Serialize();
            }
        }
    }
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

    /* ---------- 1. 先计算 X/Y 最大最小值 ---------- */
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

            // 加载并显示高度图；
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
            
            if (!bHeightTexExist) // 如果没有高度图加载空纹理
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
                        // 如果拖入的文件是高度图（Raw 文件），则设置到对应的 TerrainLayer 上
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
    const auto& terrains = m_pCurrentScene->GetTerrains();
    bool isBaking = m_bake_progress < m_bake_progress_count;

    if (isBaking) ImGui::BeginDisabled();

    if (ImGui::Button("Bake GPUTerrain data....")) 
    {
        if (!isBaking)
        {
            std::vector<TerrainNodePath> rawPaths;

            short minX = SHRT_MAX, minY = SHRT_MAX;
            short maxX = -SHRT_MAX, maxY = -SHRT_MAX;

            for (const auto& [nodeId, pTerrain] : terrains)
            {
                minX = std::min(minX, nodeId.x);
                minY = std::min(minY, nodeId.y);
                maxX = std::max(maxX, nodeId.x);
                maxY = std::max(maxY, nodeId.y);
            }

            uint32_t cntX = uint32_t(maxX - minX + 1);
            uint32_t cntY = uint32_t(maxY - minY + 1);

            for (auto& [nodeId, pTerr] : terrains)
            {
                auto* pTerrLayer = pTerr->GetTerrainLayer();
                if (!pTerrLayer)
                    throw std::runtime_error("TerrainLayer is null!");

                bool pHMapTex = pTerrLayer->GetHeightMapTexture().IsValid();
                auto& path = pHMapTex ? pTerrLayer->GetHeightMapPath() : g_defaultTex_white_wstr;

                // 写内存之前将值偏移到正数
                NXTerrainNodeId pathId = { (short)(nodeId.x - minX), (short)(nodeId.y - minY) };
                rawPaths.push_back({ pathId, path });
            }

            // 把单张地形的数据拼成完整的2D TexArray；时间比较长 用异步做
            m_bake_future = std::async(std::launch::async, [rawPaths, minX, minY, cntX, cntY, this]() {
                m_bake_progress = 0;
                m_bake_progress_count = rawPaths.size() * 3;
                std::filesystem::path outPath("D:\\NixAssets\\terrainTest\\heightMapArray.dds");
                std::filesystem::path outPath2("D:\\NixAssets\\terrainTest\\minMaxZMapArray.dds");
                std::filesystem::path outPath3("D:\\NixAssets\\terrainTest\\NormalArray.dds");

                int terrSize = 2049;
                NXTextureMaker::GenerateTerrainHeightMap2DArray(rawPaths, cntX, cntY, terrSize, terrSize, outPath, [this]() { m_bake_progress++; });
                NXTextureMaker::GenerateTerrainMinMaxZMap2DArray(rawPaths, cntX, cntY, terrSize, terrSize, outPath2, [this]() { m_bake_progress++; });
                NXTextureMaker::GenerateTerrainNormal2DArray(rawPaths, cntX, cntY, terrSize, terrSize, Vector2(0, 2048), outPath3, [this]() { m_bake_progress++; });

                NXGPUTerrainManager::GetInstance()->SetBakeTerrainTextures(outPath, outPath2, outPath3);
                NXGPUTerrainManager::GetInstance()->UpdateTerrainSupportParam(minX, minY, cntX);
                m_bNeedUpdateTerrainLayerFiles = true;
                });
        }
    }

    std::string strProgress = "Done!";
    if (isBaking)
        strProgress = "Baking..." + std::to_string(m_bake_progress) + "/" + std::to_string(m_bake_progress_count);

    ImGui::SameLine();
    ImGui::Text(strProgress.c_str());

    if (isBaking) ImGui::EndDisabled();

    std::string strShowPath = NXGPUTerrainManager::GetInstance()->GetTerrainHeightMap2DArray()->GetFilePath().string();
    ImGui::Text(strShowPath.c_str());

    ImGui::PushID("Resize for all *.raw files");
    static int val[2] = { 2049, 2049 };
    ImGui::DragInt2("", val, 1.0f, 0, 16384);
    ImGui::SameLine();
    // 批量处理Grid中录入的所有.raw文件，调整大小，并批量序列化
    if (ImGui::Button("Resize for all *.raw files"))
    {
        for (auto& [nodeId, pTerr] : terrains)
        {
            auto* pTerrLayer = pTerr->GetTerrainLayer();
            if (!pTerrLayer)
                throw std::runtime_error("TerrainLayer is null!");

            auto& pHMapTex = pTerrLayer->GetHeightMapTexture();
            if (pHMapTex.IsValid())
            {
                auto hMapData = pHMapTex->GetSerializationData();
                if (hMapData.m_rawWidth != val[0] && hMapData.m_rawHeight != val[1])
                {
                    hMapData.m_rawWidth = val[0];
                    hMapData.m_rawHeight = val[1];
                    pHMapTex->SetSerializationData(hMapData);
                    pHMapTex->Serialize();
                    pHMapTex->MarkReload(pHMapTex->GetFilePath());
                }
            }
        }
    }
    ImGui::PopID();

    ImGui::PushID("Debug Params");
    if (ImGui::DragFloat("Debug Frustum Factor", &m_debugFrustumFactor, 1.0f, 0.0f, 1000.0f))
    {
        NXGPUTerrainManager::GetInstance()->UpdateTerrainDebugParam(m_debugFrustumFactor);
    }
    ImGui::PopID();
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
