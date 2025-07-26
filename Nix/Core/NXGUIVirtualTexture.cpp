#include <algorithm>
#include <cmath>
#include <cstdio>
#include "NXGUIVirtualTexture.h"
#include "imgui.h"

NXGUIVirtualTexture::NXGUIVirtualTexture() 
{
}

void NXGUIVirtualTexture::Render()
{
    ImGui::Begin("Virtual Texture Debug");

    // 左侧列表（固定宽度）
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

    // 列表内容（Clipper 保证 6.5 万条也不卡）
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
                const auto& pos = lst[i]; // 假设 pos=(x,z) 为“右上角锚点”
                // 将右上角锚点换算为网格行列：先得到该格子的最小点(左下)
                float tileMinX = float(pos.x - SECTOR_SIZE);
                float tileMinZ = float(pos.y - SECTOR_SIZE);
                // 负数要用 floor，再转 int
                col = int(std::floor((tileMinX - WORLD_MIN) / float(SECTOR_SIZE)));
                row = int(std::floor((tileMinZ - WORLD_MIN) / float(SECTOR_SIZE)));
                // 保底夹取到合法范围
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

    // 右侧可视区域
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
    ImGui::End();
}
void NXGUIVirtualTexture::DrawWorld(ImDrawList* dl, const ImVec2& regionTL, const ImVec2& regionSize)
{
    // 计算区域右下角
    ImVec2 regionBR(regionTL.x + regionSize.x, regionTL.y + regionSize.y);

    // 背景（先铺底，不画边框；边框最后画，避免和裁剪/AA 交互导致的边缘亮线）
    dl->AddRectFilled(regionTL, regionBR, IM_COL32(30, 30, 30, 255));

    // 世界尺寸与缩放
    const float worldW = float(WORLD_MAX - WORLD_MIN); // 16384
    const float baseScale = std::min(regionSize.x / worldW, regionSize.y / worldW);
    float scale = baseScale * std::max(0.01f, m_zoom);

    // World<->Screen 映射（包含平移 m_panPix）
    auto W2S = [&](float wx, float wz) -> ImVec2 {
        float sx = regionTL.x + (wx - WORLD_MIN) * scale + m_panPix.x;
        float sy = regionTL.y + (WORLD_MAX - wz) * scale + m_panPix.y; // Z 反向
        return ImVec2(sx, sy);
        };
    auto S2W = [&](float sx, float sy) -> ImVec2 {
        float wx = WORLD_MIN + (sx - regionTL.x - m_panPix.x) / scale;
        float wz = WORLD_MAX - (sy - regionTL.y - m_panPix.y) / scale;
        return ImVec2(wx, wz);
        };

    // ===== 视图交互：中键平移 & 滚轮围绕鼠标缩放 =====
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        // 中键拖拽平移（像素坐标系，直觉稳定）
        if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            m_panPix.x += io.MouseDelta.x;
            m_panPix.y += io.MouseDelta.y;
        }

        // 鼠标滚轮缩放（以鼠标所在世界点为锚点，缩放后该点保持在同一屏幕像素位置）
        if (hovered && io.MouseWheel != 0.0f) {
            ImVec2 anchorW = S2W(io.MousePos.x, io.MousePos.y);
            float zoomFactor = (io.MouseWheel > 0.0f) ? 1.1f : 0.9f;
            float newZoom = ImClamp(m_zoom * zoomFactor, 0.25f, 4.0f);
            if (newZoom != m_zoom) {
                float newScale = baseScale * std::max(0.01f, newZoom);
                // 计算缩放后锚点的屏幕位置，并调整平移量以把锚点“拉回”到鼠标处
                ImVec2 anchorS_after = ImVec2(
                    regionTL.x + (anchorW.x - WORLD_MIN) * newScale + m_panPix.x,
                    regionTL.y + (WORLD_MAX - anchorW.y) * newScale + m_panPix.y
                );
                m_panPix.x += io.MousePos.x - anchorS_after.x;
                m_panPix.y += io.MousePos.y - anchorS_after.y;

                m_zoom = newZoom;
                scale = newScale; // 本帧立即生效
            }
        }
    }

    // ===== 裁剪区域：把网格/高亮/坐标轴都限制在内部，避免越界造成的白边 =====
    // 稍微内缩 1 像素，避免与边框 AA 混合
    ImVec2 clipTL(regionTL.x + 1.0f, regionTL.y + 1.0f);
    ImVec2 clipBR(regionBR.x - 1.0f, regionBR.y - 1.0f);
    dl->PushClipRect(clipTL, clipBR, true);

    // 像素对齐函数（保证 1px 线条清晰、不抖动）
    auto AlignPx = [](float v) -> float { return std::floor(v) + 0.5f; };

    // 网格线（可选）
    if (m_drawGrid)
    {
        ImU32 gridC = IM_COL32(80, 80, 80, 255);

        // 512 条线性能足够；有需要可根据可视世界范围计算可见索引以进一步裁剪
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

    // 世界坐标轴（也做像素对齐）
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

    // 相机附近 Sector 高亮（来自单例 Manager）
    if (auto m_mgr = NXVirtualTextureManager::GetInstance())
    {
        const auto& lst = m_mgr->GetSectorList();
        ImU32 fillC = IM_COL32(60, 160, 255, 70);
        ImU32 outlineC = IM_COL32(60, 160, 255, 200);

        for (const auto& pos : lst)
        {
            // 假定 pos 为右上角锚点 => [min, max] = [pos-64, pos]
            float minx = float(pos.x - SECTOR_SIZE);
            float maxx = float(pos.x);
            float minz = float(pos.y - SECTOR_SIZE);
            float maxz = float(pos.y);

            // 裁剪到世界范围
            if (maxx <= WORLD_MIN || minx >= WORLD_MAX || maxz <= WORLD_MIN || minz >= WORLD_MAX)
                continue;
            minx = std::max(minx, float(WORLD_MIN));
            minz = std::max(minz, float(WORLD_MIN));
            maxx = std::min(maxx, float(WORLD_MAX));
            maxz = std::min(maxz, float(WORLD_MAX));

            // 注意 Z 轴反转：左上与右下使用 (minx,maxz) 与 (maxx,minz)
            ImVec2 tl = W2S(minx, maxz);
            ImVec2 br = W2S(maxx, minz);
            dl->AddRectFilled(tl, br, fillC);

            // 像素对齐描边
            tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
            tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
            dl->AddRect(tl, br, outlineC, 0.0f, 0, 1.5f);
        }
    }

    // 选中格子的强调描边
    if (m_selectedIdx >= 0)
    {
        int row, col;
        IdxToRowCol(m_selectedIdx, row, col);
        ImVec2 mn = TileMin(row, col);
        ImVec2 mx = TileMax(row, col);
        ImVec2 tl = W2S(mn.x, mx.y);
        ImVec2 br = W2S(mx.x, mn.y);

        // 像素对齐描边
        tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
        tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
        dl->AddRect(tl, br, IM_COL32(255, 220, 60, 255), 0.0f, 0, 3.0f);
    }

    // 弹出裁剪
    dl->PopClipRect();

    // 最后绘制边框（避免与内部绘制产生边缘 AA 叠色）
    dl->AddRect(regionTL, regionBR, IM_COL32(160, 160, 160, 255));
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
    // 世界坐标（x=列，y=行->Z）
    return ImVec2(float(WORLD_MIN + col * SECTOR_SIZE),float(WORLD_MIN + row * SECTOR_SIZE));
}

ImVec2 NXGUIVirtualTexture::TileMax(int row, int col) 
{
    ImVec2 mn = TileMin(row, col);
    return ImVec2(mn.x + SECTOR_SIZE, mn.y + SECTOR_SIZE);
}
