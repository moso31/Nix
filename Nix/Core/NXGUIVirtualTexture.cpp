#include <algorithm>
#include <cmath>
#include <cstdio>
#include "NXGUIVirtualTexture.h"
#include "imgui.h"

NXGUIVirtualTexture::NXGUIVirtualTexture() 
{
    m_strTitle = {
        "Sector##child_sector",
        "Virtual image atlas##child_virtImgAtlas"
    };
}

void NXGUIVirtualTexture::Render()
{
    // -------------------- 1. �������ڣ������һ�� DockSpace�� --------------------
    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Virtual Texture Debug", &m_bShowWindow, host_flags);

    ImGuiID dockspace_id = ImGui::GetID("TerrainSystemDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);  // ����������͸��ȥ

    static bool s_dock_built = false;
    if (!s_dock_built)
    {
        s_dock_built = true;
        BuildDockLayout(dockspace_id);
    }

    Render_Sectors();
    Render_VirtImageAtlas();

    ImGui::End(); // Virtual Texture Debug
}

void NXGUIVirtualTexture::Render_Sectors()
{
    ImGui::Begin(m_strTitle[0].c_str());

    // ����б��̶���ȣ�
    const float kLeftWidth = 360.0f;
    ImGui::BeginChild("LeftList", ImVec2(kLeftWidth, 0.0f), true);

    auto m_mgr = NXVirtualTextureManager::GetInstance();
    int nearCount = (m_mgr ? (int)m_mgr->GetSectorList().size() : 0);
    ImGui::Text("Near sector: %d", nearCount);

    ImGui::Separator();
    ImGui::Checkbox("Show only near Sector", &m_showOnlyNear);
    ImGui::Checkbox("Draw grid", &m_drawGrid);
    ImGui::SliderFloat("Scale", &m_zoom, 0.25f, 4.0f, "%.2fx");
    if (ImGui::Button("Reset view")) {
        m_zoom = 1.0f;
        m_panPix = ImVec2(0.0f, 0.0f);
    }
    ImGui::Separator();

    // �б����ݣ�Clipper ��֤ 6.5 ����Ҳ������
    const int total = (m_showOnlyNear && m_mgr) ? nearCount : GRID_COUNT * GRID_COUNT;
    ImGui::Text("list(%d)", total);
    ImGui::Separator();

    ImGuiListClipper clipper;
    clipper.Begin(total);
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            int row, col;
            ImVec2 mn, mx;
            int globalIdx = -1;

            if (m_showOnlyNear && m_mgr)
            {
                const auto& lst = m_mgr->GetSectorList();
                const auto& sectorInfo = lst[i]; // ���� pos=(x,z) Ϊ�����Ͻ�ê�㡱
                // �����Ͻ�ê�㻻��Ϊ�������У��ȵõ��ø��ӵ���С��(����)
                float tileMinX = float(sectorInfo.x - SECTOR_SIZE);
                float tileMinZ = float(sectorInfo.y - SECTOR_SIZE);
                // ����Ҫ�� floor����ת int
                col = int(std::floor((tileMinX - WORLD_MIN) / float(SECTOR_SIZE)));
                row = int(std::floor((tileMinZ - WORLD_MIN) / float(SECTOR_SIZE)));
                // ���׼�ȡ���Ϸ���Χ
                col = std::clamp(col, 0, GRID_COUNT - 1);
                row = std::clamp(row, 0, GRID_COUNT - 1);

                mn = ImVec2(tileMinX, tileMinZ);
                mx = ImVec2(tileMinX + SECTOR_SIZE, tileMinZ + SECTOR_SIZE);
                globalIdx = RowColToIdx(row, col);
            }
            else
            {
                IdxToRowCol(i, row, col);
                mn = TileMin(row, col);
                mx = TileMax(row, col);
                globalIdx = i;
            }

            char label[160];
            std::snprintf(label, sizeof(label),
                "R%03d C%03d  Min(%.0f,%.0f)  Max(%.0f,%.0f)",
                row, col, mn.x, mn.y, mx.x, mx.y);

            bool selected = (m_selectedIdx == globalIdx);
            if (ImGui::Selectable(label, selected))
                m_selectedIdx = globalIdx;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // �Ҳ��������
    ImGui::BeginChild("ViewRegion",
        ImVec2(0.0f, 0.0f),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 viewTL = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 pad(8.0f, 8.0f);
    ImVec2 regionTL(viewTL.x + pad.x, viewTL.y + pad.y);
    ImVec2 regionSize(avail.x - pad.x * 2.0f, avail.y - pad.y * 2.0f);

    DrawWorld(dl, regionTL, regionSize);

    ImGui::EndChild();
    ImGui::End(); // child_sector
}

void NXGUIVirtualTexture::Render_VirtImageAtlas()
{
    ImGui::Begin(m_strTitle[1].c_str());

    // ---- ȡ���� ----
    auto* m_mgr = NXVirtualTextureManager::GetInstance();
    if (!m_mgr)
    {
        ImGui::TextUnformatted("NXVirtualTextureManager::GetInstance() == nullptr");
        ImGui::End();
        return;
    }

    const auto& nodes = m_mgr->GetNodes();          // ����չ����һά�ڵ�����
    const int totalNodes = (int)nodes.size();

    // ---- ���Ҳ��֣����б� + ����ͼ ----
    const float kLeftWidth = 360.0f;
    ImGui::BeginChild("AtlasLeftList", ImVec2(kLeftWidth, 0.0f), true);

    // ����ѡ����״̬�������� Atlas����Ӱ�� Sector��
    static bool  s_drawGrid = true;    // �Ҳ���ͼ�Ƿ񻭲ο�����
    static float s_zoom = 1.0f;    // �Ҳ�����
    static ImVec2 s_panPix = ImVec2(0.0f, 0.0f); // �Ҳ�ƽ�ƣ�����ϵ��
    static int   s_selectedIdx = -1;      // ѡ�е���������
    constexpr int ATLAS_SIZE = 2048;       // ������ʸ��ͼ���߳�

    // ͳ��
    int imageCount = 0;
    for (int i = 0; i < totalNodes; ++i)
    {
        const auto& n = nodes[i];
        if (n->isImage) imageCount++;
    }

    ImGui::Text("Atlas size: %dx%d", ATLAS_SIZE, ATLAS_SIZE);
    ImGui::Text("Nodes: %d", totalNodes);
    ImGui::Separator();
    ImGui::Checkbox("Draw grid in view", &s_drawGrid);
    ImGui::SliderFloat("Scale", &s_zoom, 0.25f, 16.0f, "%.2fx");
    if (ImGui::Button("Reset view"))
    {
        s_zoom = 1.0f;
        s_panPix = ImVec2(0.0f, 0.0f);
    }
    ImGui::Separator();
    auto pRoot = m_mgr->GetAtlasRootNode();
    ImGui::Text("rootSubImgNum: %d", pRoot->subImageNum);
    ImGui::Separator();

    // �б�ʹ�� clipper
    {
        // ��ȷ���б���Ҫ��ʾ����Ŀ�������ϣ������� clipper �ڲ��������жϣ�
        static std::vector<int> s_viewIndices;
        s_viewIndices.clear();
        s_viewIndices.reserve(totalNodes);

        for (int i = 0; i < totalNodes; ++i)
        {
            const auto& n = nodes[i];
            s_viewIndices.push_back(i);
        }

        ImGui::Text("list(%d)", (int)s_viewIndices.size());
        ImGui::Separator();

        ImGuiListClipper clipper;
        clipper.Begin((int)s_viewIndices.size());
        while (clipper.Step())
        {
            for (int k = clipper.DisplayStart; k < clipper.DisplayEnd; ++k)
            {
                const int i = s_viewIndices[k];
                const auto& n = nodes[i];

                char buf[192];
                std::snprintf(buf, sizeof(buf),
                    "#%-5d pos(%4d,%4d) size(%4d): %d, %d", 
                    n->nodeID,
                    n->position.x, n->position.y,
                    n->size, 
                    n->isImage,
                    n->subImageNum
                    );

                bool selected = (s_selectedIdx == i);
                if (ImGui::Selectable(buf, selected))
                    s_selectedIdx = i;
            }
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // ---- �Ҳࣺ���ӻ����� ----
    ImGui::BeginChild("AtlasView",
        ImVec2(0.0f, 0.0f),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 viewTL = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 pad(8.0f, 8.0f);
    ImVec2 regionTL(viewTL.x + pad.x, viewTL.y + pad.y);
    ImVec2 regionSize(avail.x - pad.x * 2.0f, avail.y - pad.y * 2.0f);
    ImVec2 regionBR(regionTL.x + regionSize.x, regionTL.y + regionSize.y);

    // ����
    dl->AddRectFilled(regionTL, regionBR, IM_COL32(30, 30, 30, 255));

    // Atlas<->Screen ӳ��
    const float baseScale = std::min(regionSize.x / float(ATLAS_SIZE), regionSize.y / float(ATLAS_SIZE));
    float scale = baseScale * std::max(0.01f, s_zoom);

    auto A2S = [&](float ax, float ay) -> ImVec2 {
        // Atlas ԭ�������ϣ�Y ���£�����Ļһ�£�
        float sx = regionTL.x + ax * scale + s_panPix.x;
        float sy = regionTL.y + ay * scale + s_panPix.y;
        return ImVec2(sx, sy);
        };
    auto S2A = [&](float sx, float sy) -> ImVec2 {
        float ax = (sx - regionTL.x - s_panPix.x) / scale;
        float ay = (sy - regionTL.y - s_panPix.y) / scale;
        return ImVec2(ax, ay);
        };

    // ��ͼ�������м�ƽ�ơ�����Χ���������
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            s_panPix.x += io.MouseDelta.x;
            s_panPix.y += io.MouseDelta.y;
        }

        if (hovered && io.MouseWheel != 0.0f) {
            ImVec2 anchorA = S2A(io.MousePos.x, io.MousePos.y);
            float zoomFactor = (io.MouseWheel > 0.0f) ? 1.1f : 0.9f;
            float newZoom = ImClamp(s_zoom * zoomFactor, 0.25f, 16.0f);
            if (newZoom != s_zoom) {
                float newScale = baseScale * std::max(0.01f, newZoom);
                ImVec2 anchorS_after = ImVec2(
                    regionTL.x + anchorA.x * newScale + s_panPix.x,
                    regionTL.y + anchorA.y * newScale + s_panPix.y
                );
                s_panPix.x += io.MousePos.x - anchorS_after.x;
                s_panPix.y += io.MousePos.y - anchorS_after.y;

                s_zoom = newZoom;
                scale = newScale; // ��֡��Ч
            }
        }
    }

    // �ü�
    ImVec2 clipTL(regionTL.x + 1.0f, regionTL.y + 1.0f);
    ImVec2 clipBR(regionBR.x - 1.0f, regionBR.y - 1.0f);
    dl->PushClipRect(clipTL, clipBR, true);

    auto AlignPx = [](float v) -> float { return std::floor(v) + 0.5f; };

    // �ο����񣨿�ѡ��
    if (s_drawGrid)
    {
        ImU32 gridC = IM_COL32(70, 70, 70, 255);
        // ÿ 128 ��һ��������̫�ܣ��㹻�ο���
        const int step = 128;
        for (int k = 0; k <= ATLAS_SIZE; k += step)
        {
            ImVec2 v0 = A2S((float)k, 0.0f);
            ImVec2 v1 = A2S((float)k, (float)ATLAS_SIZE);
            v0.x = AlignPx(v0.x); v1.x = AlignPx(v1.x);
            dl->AddLine(v0, v1, gridC);

            ImVec2 h0 = A2S(0.0f, (float)k);
            ImVec2 h1 = A2S((float)ATLAS_SIZE, (float)k);
            h0.y = AlignPx(h0.y); h1.y = AlignPx(h1.y);
            dl->AddLine(h0, h1, gridC);
        }
    }

    // ��ʱ��������¼nodeת��������λ�úʹ�С
    Int2 pos;
    int size;

    // ����������Ч�ڵ㣺�ڲ��ڵ㻭�߿�isImage ��Ҷ�ӽڵ��ټ���΢���
    {
        for (int i = 0; i < totalNodes; ++i)
        {
            const auto& n = nodes[i];
            ImU32 imageFill = GetSectorRectColor(n->size, true);
            ImU32 imageLine = GetSectorRectColor(n->size, false);

            m_mgr->GetImagePosAndSize(n, pos, size);
            ImVec2 tl = A2S((float)pos.x - (float)size, (float)pos.y - (float)size);
            ImVec2 br = A2S((float)pos.x + (float)size, (float)pos.y + (float)size);

            // ������+�߿�
            ImVec2 atl = tl, abr = br;
            atl.x = AlignPx(atl.x); abr.x = AlignPx(abr.x);
            atl.y = AlignPx(atl.y); abr.y = AlignPx(abr.y);
            dl->AddRectFilled(tl, br, imageFill);
            dl->AddRect(atl, abr, imageLine, 0.0f, 0, 1.5f);
        }

        // ѡ�и���
        if (s_selectedIdx >= 0 && s_selectedIdx < totalNodes)
        {
            const auto& n = nodes[s_selectedIdx];
            m_mgr->GetImagePosAndSize(n, pos, size);
            ImVec2 tl = A2S((float)pos.x - (float)size, (float)pos.y - (float)size);
            ImVec2 br = A2S((float)pos.x + (float)size, (float)pos.y + (float)size);
            tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
            tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
            dl->AddRect(tl, br, IM_COL32(255, 220, 60, 255), 0.0f, 0, 3.0f);
        }
    }

    // �����ü���������߿�
    dl->PopClipRect();
    dl->AddRect(regionTL, regionBR, IM_COL32(160, 160, 160, 255));

    ImGui::EndChild();
    ImGui::End();
}


void NXGUIVirtualTexture::BuildDockLayout(ImGuiID dockspace_id)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoSplit);  
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

    ImGui::DockBuilderDockWindow(m_strTitle[0].c_str(), dockspace_id);
    ImGui::DockBuilderDockWindow(m_strTitle[1].c_str(), dockspace_id);

    ImGui::DockBuilderFinish(dockspace_id);  // ��ɣ�
}

void NXGUIVirtualTexture::DrawWorld(ImDrawList* dl, const ImVec2& regionTL, const ImVec2& regionSize)
{
    // �����������½�
    ImVec2 regionBR(regionTL.x + regionSize.x, regionTL.y + regionSize.y);

    // ���������̵ף������߿򣻱߿���󻭣�����Ͳü�/AA �������µı�Ե���ߣ�
    dl->AddRectFilled(regionTL, regionBR, IM_COL32(30, 30, 30, 255));

    // ����ߴ�������
    const float worldW = float(WORLD_MAX - WORLD_MIN); // 16384
    const float baseScale = std::min(regionSize.x / worldW, regionSize.y / worldW);
    float scale = baseScale * std::max(0.01f, m_zoom);

    // World<->Screen ӳ�䣨����ƽ�� m_panPix��
    auto W2S = [&](float wx, float wz) -> ImVec2 {
        float sx = regionTL.x + (wx - WORLD_MIN) * scale + m_panPix.x;
        float sy = regionTL.y + (WORLD_MAX - wz) * scale + m_panPix.y; // Z ����
        return ImVec2(sx, sy);
        };
    auto S2W = [&](float sx, float sy) -> ImVec2 {
        float wx = WORLD_MIN + (sx - regionTL.x - m_panPix.x) / scale;
        float wz = WORLD_MAX - (sy - regionTL.y - m_panPix.y) / scale;
        return ImVec2(wx, wz);
        };

    // ===== ��ͼ�������м�ƽ�� & ����Χ��������� =====
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        // �м���קƽ�ƣ���������ϵ��ֱ���ȶ���
        if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            m_panPix.x += io.MouseDelta.x;
            m_panPix.y += io.MouseDelta.y;
        }

        // ���������ţ���������������Ϊê�㣬���ź�õ㱣����ͬһ��Ļ����λ�ã�
        if (hovered && io.MouseWheel != 0.0f) {
            ImVec2 anchorW = S2W(io.MousePos.x, io.MousePos.y);
            float zoomFactor = (io.MouseWheel > 0.0f) ? 1.1f : 0.9f;
            float newZoom = ImClamp(m_zoom * zoomFactor, 0.25f, 4.0f);
            if (newZoom != m_zoom) {
                float newScale = baseScale * std::max(0.01f, newZoom);
                // �������ź�ê�����Ļλ�ã�������ƽ�����԰�ê�㡰���ء�����괦
                ImVec2 anchorS_after = ImVec2(
                    regionTL.x + (anchorW.x - WORLD_MIN) * newScale + m_panPix.x,
                    regionTL.y + (WORLD_MAX - anchorW.y) * newScale + m_panPix.y
                );
                m_panPix.x += io.MousePos.x - anchorS_after.x;
                m_panPix.y += io.MousePos.y - anchorS_after.y;

                m_zoom = newZoom;
                scale = newScale; // ��֡������Ч
            }
        }
    }

    // ===== �ü����򣺰�����/����/�����ᶼ�������ڲ�������Խ����ɵİױ� =====
    // ��΢���� 1 ���أ�������߿� AA ���
    ImVec2 clipTL(regionTL.x + 1.0f, regionTL.y + 1.0f);
    ImVec2 clipBR(regionBR.x - 1.0f, regionBR.y - 1.0f);
    dl->PushClipRect(clipTL, clipBR, true);

    // ���ض��뺯������֤ 1px ������������������
    auto AlignPx = [](float v) -> float { return std::floor(v) + 0.5f; };

    // �����ߣ���ѡ��
    if (m_drawGrid)
    {
        ImU32 gridC = IM_COL32(80, 80, 80, 255);

        // 512 ���������㹻������Ҫ�ɸ��ݿ������緶Χ����ɼ������Խ�һ���ü�
        for (int k = 0; k <= GRID_COUNT; ++k)
        {
            float wx = float(WORLD_MIN + k * SECTOR_SIZE);
            ImVec2 v0 = W2S(wx, WORLD_MIN);
            ImVec2 v1 = W2S(wx, WORLD_MAX);
            v0.x = AlignPx(v0.x); v1.x = AlignPx(v1.x);
            dl->AddLine(v0, v1, gridC);

            float wz = float(WORLD_MIN + k * SECTOR_SIZE);
            ImVec2 h0 = W2S(WORLD_MIN, wz);
            ImVec2 h1 = W2S(WORLD_MAX, wz);
            h0.y = AlignPx(h0.y); h1.y = AlignPx(h1.y);
            dl->AddLine(h0, h1, gridC);
        }
    }

    // ���������ᣨҲ�����ض��룩
    {
        ImVec2 x0 = W2S(0.0f, WORLD_MIN);
        ImVec2 x1 = W2S(0.0f, WORLD_MAX);
        x0.x = AlignPx(x0.x); x1.x = AlignPx(x1.x);
        dl->AddLine(x0, x1, IM_COL32(60, 60, 200, 255), 2.0f); // X=0

        ImVec2 z0 = W2S(WORLD_MIN, 0.0f);
        ImVec2 z1 = W2S(WORLD_MAX, 0.0f);
        z0.y = AlignPx(z0.y); z1.y = AlignPx(z1.y);
        dl->AddLine(z0, z1, IM_COL32(200, 60, 60, 255), 2.0f); // Z=0
    }

    // ������� Sector ���������Ե��� Manager��
    if (auto m_mgr = NXVirtualTextureManager::GetInstance())
    {
        const auto& lst = m_mgr->GetSectorInfos();

        for (const auto& sectorInfo : lst)
        {
            ImU32 imageFill = GetSectorRectColor(sectorInfo.size, true);
            ImU32 imageLine = GetSectorRectColor(sectorInfo.size, false);

            float minx = float(sectorInfo.position.x - SECTOR_SIZE);
            float maxx = float(sectorInfo.position.x);
            float minz = float(sectorInfo.position.y - SECTOR_SIZE);
            float maxz = float(sectorInfo.position.y);

            // �ü������緶Χ
            if (maxx <= WORLD_MIN || minx >= WORLD_MAX || maxz <= WORLD_MIN || minz >= WORLD_MAX)
                continue;
            minx = std::max(minx, float(WORLD_MIN));
            minz = std::max(minz, float(WORLD_MIN));
            maxx = std::min(maxx, float(WORLD_MAX));
            maxz = std::min(maxz, float(WORLD_MAX));

            // ע�� Z �ᷴת������������ʹ�� (minx,maxz) �� (maxx,minz)
            ImVec2 tl = W2S(minx, maxz);
            ImVec2 br = W2S(maxx, minz);
            dl->AddRectFilled(tl, br, imageFill);

            // ���ض������
            tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
            tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
            dl->AddRect(tl, br, imageLine, 0.0f, 0, 1.5f);
        }
    }

    // ѡ�и��ӵ�ǿ�����
    if (m_selectedIdx >= 0)
    {
        int row, col;
        IdxToRowCol(m_selectedIdx, row, col);
        ImVec2 mn = TileMin(row, col);
        ImVec2 mx = TileMax(row, col);
        ImVec2 tl = W2S(mn.x, mx.y);
        ImVec2 br = W2S(mx.x, mn.y);

        // ���ض������
        tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
        tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
        dl->AddRect(tl, br, IM_COL32(255, 220, 60, 255), 0.0f, 0, 3.0f);
    }

    // �����ü�
    dl->PopClipRect();

    // �����Ʊ߿򣨱������ڲ����Ʋ�����Ե AA ��ɫ��
    dl->AddRect(regionTL, regionBR, IM_COL32(160, 160, 160, 255));
}

ImU32 NXGUIVirtualTexture::GetSectorRectColor(int size, bool isFill)
{
    const int alpha = isFill ? 70 : 220; // �����������͸��һЩ

    switch (size)
    {
    case 256: return IM_COL32(255, 60, 60, alpha); // ��
    case 128: return IM_COL32(60, 255, 60, alpha); // ��
    case  64: return IM_COL32(60, 160, 255, alpha); // ԭ�� 256 ��ƫ��
    case  32: return IM_COL32(60, 255, 255, alpha); // ��
    case  16: return IM_COL32(255, 60, 255, alpha); // Ʒ��
    case   8: return IM_COL32(255, 160, 60, alpha); // ��
    case   4: return IM_COL32(160, 60, 255, alpha); // ��
    case   2: return IM_COL32(60, 255, 160, alpha); // ��ȸ��/����
    case   1: return IM_COL32(255, 255, 60, alpha); // ��
    default:  return IM_COL32(255, 255, 255, alpha); // Ĭ�ϰ�ɫ
    }
}


void NXGUIVirtualTexture::IdxToRowCol(int idx, int& row, int& col)
{
    row = idx / GRID_COUNT;
    col = idx % GRID_COUNT;
}

int NXGUIVirtualTexture::RowColToIdx(int row, int col)
{
	return row * GRID_COUNT + col;
}

ImVec2 NXGUIVirtualTexture::TileMin(int row, int col)
{ 
    // �������꣨x=�У�y=��->Z��
    return ImVec2(float(WORLD_MIN + col * SECTOR_SIZE),float(WORLD_MIN + row * SECTOR_SIZE));
}

ImVec2 NXGUIVirtualTexture::TileMax(int row, int col) 
{
    ImVec2 mn = TileMin(row, col);
    return ImVec2(mn.x + SECTOR_SIZE, mn.y + SECTOR_SIZE);
}
