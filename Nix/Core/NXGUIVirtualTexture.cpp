#include <algorithm>
#include <cmath>
#include <cstdio>
#include "imgui.h"
#include "NXGUIVirtualTexture.h"
#include "NXGlobalDefinitions.h"
#include "Renderer.h"
#include "NXVirtualTexture.h"
#include "NXVTImageQuadTree.h"

// 缓存数据结构体定义
struct NXGUIVirtualTexture::CachedData
{
    std::vector<NXVTSector> sectors;
    std::unordered_map<NXVTSector, Int2> sector2VirtImagePos;
};

NXVirtualTexture* NXGUIVirtualTexture::GetVirtualTexture() const
{
    if (!m_pOwner) return nullptr;
    auto* pRenderer = m_pOwner->GetRenderer();
    if (!pRenderer) return nullptr;
    return pRenderer->GetVirtualTexture();
}

NXGUIVirtualTexture::NXGUIVirtualTexture(NXGUI* pOwner) :
    m_pOwner(pOwner),
    m_bShowWindow(false),
    m_cachedData(new CachedData())
{
    m_strTitle = {
        "Sector##child_sector",
        "Virtual image atlas##child_virtImgAtlas",
        "Readback##child_readback"
    };
}

NXGUIVirtualTexture::~NXGUIVirtualTexture() = default;

void NXGUIVirtualTexture::Render()
{
    // 如果窗口关闭，直接返回，避免不必要的渲染开销
    if (!m_bShowWindow)
        return;

    // -------------------- 1. 宿主窗口（里面放一个 DockSpace） --------------------
    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Virtual Texture Debug", &m_bShowWindow, host_flags);

    ImGuiID dockspace_id = ImGui::GetID("TerrainSystemDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);  // 让主窗背景透过去

    static bool s_dock_built = false;
    if (!s_dock_built)
    {
        s_dock_built = true;
        BuildDockLayout(dockspace_id);
    }

    Render_Sectors();
    Render_VirtImageAtlas();
    Render_Readback();

    ImGui::End(); // Virtual Texture Debug
}

void NXGUIVirtualTexture::Render_Sectors()
{
    ImGui::Begin(m_strTitle[0].c_str());

    // 左侧列表（固定宽度）
    const float kLeftWidth = 360.0f;
    ImGui::BeginChild("LeftList", ImVec2(kLeftWidth, 0.0f), true);

    auto* pVT = GetVirtualTexture();
    // 只在Finish状态时更新缓存，避免中间帧数据不一致
    if (pVT && pVT->GetUpdateState() == NXVTUpdateState::Finish)
    {
        m_cachedData->sectors = pVT->GetSectors();
    }
    int nearCount = (int)m_cachedData->sectors.size();
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
    const int total = (m_showOnlyNear && pVT) ? nearCount : GRID_COUNT * GRID_COUNT;
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

            if (m_showOnlyNear)
            {
                const auto& lst = m_cachedData->sectors;
                const auto& sectorInfo = lst[i];
                // sectorInfo.id 是 sector 的网格坐标
                // 转换为世界坐标：sectorPos = id * SECTOR_SIZE
                float tileMinX = float(sectorInfo.id.x * SECTOR_SIZE);
                float tileMinZ = float(sectorInfo.id.y * SECTOR_SIZE);
                // 计算行列
                col = sectorInfo.id.x - (WORLD_MIN / SECTOR_SIZE);
                row = sectorInfo.id.y - (WORLD_MIN / SECTOR_SIZE);
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
    ImGui::End(); // child_sector
}

void NXGUIVirtualTexture::Render_VirtImageAtlas()
{
    ImGui::Begin(m_strTitle[1].c_str());

    // ---- 取数据 ----
    auto* pVT = GetVirtualTexture();
    if (!pVT)
    {
        ImGui::TextUnformatted("NXVirtualTexture is nullptr");
        ImGui::End();
        return;
    }

    const auto* pQuadTree = pVT->GetQuadTree();
    if (!pQuadTree)
    {
        ImGui::TextUnformatted("QuadTree is nullptr");
        ImGui::End();
        return;
    }

    // 只在Finish状态时更新缓存，避免中间帧数据不一致
    if (pVT->GetUpdateState() == NXVTUpdateState::Finish)
    {
        m_cachedData->sectors = pVT->GetSectors();
        m_cachedData->sector2VirtImagePos = pVT->GetSector2VirtImagePos();
    }

    // 获取所有已分配的叶子节点（sector到VirtImage位置的映射）
    const auto& sector2Pos = m_cachedData->sector2VirtImagePos;
    const auto& sectors = m_cachedData->sectors;
    const int ATLAS_SIZE = pQuadTree->GetImageSize(); // 2048

    // ---- 左右布局：左列表 + 右视图 ----
    const float kLeftWidth = 360.0f;
    ImGui::BeginChild("AtlasLeftList", ImVec2(kLeftWidth, 0.0f), true);

    // 交互选项与状态
    static bool  s_drawGrid = true;
    static float s_zoom = 1.0f;
    static ImVec2 s_panPix = ImVec2(0.0f, 0.0f);
    static int   s_selectedIdx = -1;

    ImGui::Text("Atlas size: %dx%d", ATLAS_SIZE, ATLAS_SIZE);
    ImGui::Text("Leaf nodes (allocated): %d", (int)sector2Pos.size());
    ImGui::Separator();
    
    // 搜索框
    ImGui::Text("Search:");
    ImGui::SameLine();
    if (ImGui::InputText("##AtlasSearch", m_atlasSearchBuf, sizeof(m_atlasSearchBuf)))
    {
        // 搜索框内容变化时的处理
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        m_atlasSearchBuf[0] = '\0';
    }
    ImGui::Separator();
    
    ImGui::Checkbox("Draw grid in view", &s_drawGrid);
    ImGui::SliderFloat("Scale", &s_zoom, 0.25f, 16.0f, "%.2fx");
    if (ImGui::Button("Reset view"))
    {
        s_zoom = 1.0f;
        s_panPix = ImVec2(0.0f, 0.0f);
    }
    ImGui::Separator();

    // 过滤列表：根据搜索字符串过滤
    std::vector<int> filteredIndices;
    std::string searchStr = m_atlasSearchBuf;
    bool hasSearch = !searchStr.empty();
    
    if (hasSearch)
    {
        // 有搜索条件，过滤节点
        for (int i = 0; i < (int)sectors.size(); ++i)
        {
            const auto& sector = sectors[i];
            auto it = sector2Pos.find(sector);
            
            char buf[256];
            if (it != sector2Pos.end())
            {
                const Int2& virtPos = it->second;
                int pixelX = virtPos.x * sector.imageSize;
                int pixelY = virtPos.y * sector.imageSize;
                std::snprintf(buf, sizeof(buf), 
                    "Sector(%d,%d) -> VirtImg pos(%d,%d) size=%d", 
                    sector.id.x, sector.id.y, 
                    pixelX, pixelY, sector.imageSize);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), 
                    "Sector(%d,%d) size=%d [no mapping]", 
                    sector.id.x, sector.id.y, sector.imageSize);
            }
            
            // 检查是否包含搜索字符串（不区分大小写）
            std::string bufStr = buf;
            std::string searchLower = searchStr;
            std::string bufLower = bufStr;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
            std::transform(bufLower.begin(), bufLower.end(), bufLower.begin(), ::tolower);
            
            if (bufLower.find(searchLower) != std::string::npos)
            {
                filteredIndices.push_back(i);
            }
        }
    }
    else
    {
        // 没有搜索条件，显示所有节点
        filteredIndices.resize(sectors.size());
        for (int i = 0; i < (int)sectors.size(); ++i)
        {
            filteredIndices[i] = i;
        }
    }
    
    // 列表：显示过滤后的叶子节点
    int totalCount = (int)sectors.size();
    int filteredCount = (int)filteredIndices.size();
    if (hasSearch)
    {
        ImGui::Text("Leaf nodes list (%d / %d)", filteredCount, totalCount);
    }
    else
    {
        ImGui::Text("Leaf nodes list (%d)", totalCount);
    }
    ImGui::Separator();

    ImGuiListClipper clipper;
    clipper.Begin(filteredCount);
    while (clipper.Step())
    {
        for (int listIdx = clipper.DisplayStart; listIdx < clipper.DisplayEnd; ++listIdx)
        {
            int i = filteredIndices[listIdx];
            const auto& sector = sectors[i];
            auto it = sector2Pos.find(sector);
            
            char buf[256];
            if (it != sector2Pos.end())
            {
                const Int2& virtPos = it->second;
                // virtPos 是四叉树索引转换后的位置，需要乘以imageSize得到实际像素坐标
                int pixelX = virtPos.x * sector.imageSize;
                int pixelY = virtPos.y * sector.imageSize;
                std::snprintf(buf, sizeof(buf), 
                    "Sector(%d,%d) -> VirtImg pos(%d,%d) size=%d", 
                    sector.id.x, sector.id.y, 
                    pixelX, pixelY, sector.imageSize);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), 
                    "Sector(%d,%d) size=%d [no mapping]", 
                    sector.id.x, sector.id.y, sector.imageSize);
            }

            bool selected = (s_selectedIdx == i);
            if (ImGui::Selectable(buf, selected))
                s_selectedIdx = i;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // ---- 右侧：可视化绘制 ----
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

    // 背景
    dl->AddRectFilled(regionTL, regionBR, IM_COL32(30, 30, 30, 255));

    // Atlas<->Screen 映射
    const float baseScale = std::min(regionSize.x / float(ATLAS_SIZE), regionSize.y / float(ATLAS_SIZE));
    float scale = baseScale * std::max(0.01f, s_zoom);

    auto A2S = [&](float ax, float ay) -> ImVec2 {
        float sx = regionTL.x + ax * scale + s_panPix.x;
        float sy = regionTL.y + ay * scale + s_panPix.y;
        return ImVec2(sx, sy);
    };
    auto S2A = [&](float sx, float sy) -> ImVec2 {
        float ax = (sx - regionTL.x - s_panPix.x) / scale;
        float ay = (sy - regionTL.y - s_panPix.y) / scale;
        return ImVec2(ax, ay);
    };

    // 视图交互：中键平移、滚轮围绕鼠标缩放
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
                scale = newScale;
            }
        }
    }

    // 裁剪
    ImVec2 clipTL(regionTL.x + 1.0f, regionTL.y + 1.0f);
    ImVec2 clipBR(regionBR.x - 1.0f, regionBR.y - 1.0f);
    dl->PushClipRect(clipTL, clipBR, true);

    auto AlignPx = [](float v) -> float { return std::floor(v) + 0.5f; };

    // 参考网格（可选）
    if (s_drawGrid)
    {
        ImU32 gridC = IM_COL32(70, 70, 70, 255);
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

    // 绘制所有叶子节点（已分配的sector在VirtImage中的位置）
    {
        int idx = 0;
        for (const auto& sector : sectors)
        {
            auto it = sector2Pos.find(sector);
            if (it == sector2Pos.end())
            {
                idx++;
                continue;
            }

            const Int2& virtPos = it->second;
            int nodeSize = sector.imageSize;
            // virtPos是四叉树位置索引，乘以nodeSize得到像素坐标
            float x = (float)(virtPos.x * nodeSize);
            float y = (float)(virtPos.y * nodeSize);

            ImU32 fillColor = GetSectorRectColor(nodeSize, true);
            ImU32 lineColor = GetSectorRectColor(nodeSize, false);

            ImVec2 tl = A2S(x, y);
            ImVec2 br = A2S(x + nodeSize, y + nodeSize);

            // 画矩形+边框
            ImVec2 atl = tl, abr = br;
            atl.x = AlignPx(atl.x); abr.x = AlignPx(abr.x);
            atl.y = AlignPx(atl.y); abr.y = AlignPx(abr.y);
            dl->AddRectFilled(tl, br, fillColor);
            dl->AddRect(atl, abr, lineColor, 0.0f, 0, 1.5f);

            idx++;
        }

        // 选中高亮
        if (s_selectedIdx >= 0 && s_selectedIdx < (int)sectors.size())
        {
            const auto& sector = sectors[s_selectedIdx];
            auto it = sector2Pos.find(sector);
            if (it != sector2Pos.end())
            {
                const Int2& virtPos = it->second;
                int nodeSize = sector.imageSize;
                float x = (float)(virtPos.x * nodeSize);
                float y = (float)(virtPos.y * nodeSize);
                ImVec2 tl = A2S(x, y);
                ImVec2 br = A2S(x + nodeSize, y + nodeSize);
                tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
                tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
                dl->AddRect(tl, br, IM_COL32(255, 220, 60, 255), 0.0f, 0, 3.0f);
            }
        }
    }

    // 弹出裁剪并画最外边框
    dl->PopClipRect();
    dl->AddRect(regionTL, regionBR, IM_COL32(160, 160, 160, 255));

    ImGui::EndChild();
    ImGui::End();
}

void NXGUIVirtualTexture::Render_Readback()
{
    ImGui::Begin(m_strTitle[2].c_str());

    // 数据与尺寸
    NXVirtualTexture* pVT = GetVirtualTexture();
    auto& vtReadbackData = pVT->GetVTReadbackData()->Get();
    const uint32_t* pVTData = reinterpret_cast<const uint32_t*>(vtReadbackData.data());
    const Int2 sz = pVT->GetVTReadbackDataSize();
    const int W = sz.x, H = sz.y;

    ImGui::Text("Readback size: %d x %d", W, H);
    if (W <= 0 || H <= 0 || !pVTData || vtReadbackData.empty()) {
        ImGui::TextUnformatted("No data.");
        ImGui::End();
        return;
    }

    // 简单控制（仅缩放/平移；可删）
    static float  s_zoom = 1.0f;
    static ImVec2 s_panPix = ImVec2(0, 0);
    if (ImGui::SliderFloat("Scale", &s_zoom, 0.25f, 32.0f, "%.2fx")) {}
    ImGui::SameLine();
    if (ImGui::Button("Reset view")) { s_zoom = 1.0f; s_panPix = ImVec2(0, 0); }

    // 视图区域
    ImGui::BeginChild("RBView", ImVec2(0, 0), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 viewTL = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 pad(8, 8);
    ImVec2 regionTL(viewTL.x + pad.x, viewTL.y + pad.y);
    ImVec2 regionSize(avail.x - pad.x * 2, avail.y - pad.y * 2);
    ImVec2 regionBR(regionTL.x + regionSize.x, regionTL.y + regionSize.y);

    dl->AddRectFilled(regionTL, regionBR, IM_COL32(30, 30, 30, 255));

    // 坐标变换：像素坐标 <-> 屏幕
    const float baseScale = std::min(regionSize.x / float(W), regionSize.y / float(H));
    float scale = baseScale * std::max(0.01f, s_zoom);

    auto A2S = [&](float ax, float ay)->ImVec2 {
        return ImVec2(regionTL.x + ax * scale + s_panPix.x,
            regionTL.y + ay * scale + s_panPix.y);
        };
    auto S2A = [&](float sx, float sy)->ImVec2 {
        return ImVec2((sx - regionTL.x - s_panPix.x) / scale,
            (sy - regionTL.y - s_panPix.y) / scale);
        };

    // 简单交互：中键平移、滚轮缩放（围绕鼠标）
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
            float newZoom = ImClamp(s_zoom * zoomFactor, 0.25f, 64.0f);
            if (newZoom != s_zoom) {
                float newScale = baseScale * std::max(0.01f, newZoom);
                ImVec2 anchorS_after = ImVec2(
                    regionTL.x + anchorA.x * newScale + s_panPix.x,
                    regionTL.y + anchorA.y * newScale + s_panPix.y
                );
                s_panPix.x += io.MousePos.x - anchorS_after.x;
                s_panPix.y += io.MousePos.y - anchorS_after.y;
                s_zoom = newZoom;
                scale = newScale;
            }
        }
    }

    // 裁剪
    dl->PushClipRect(ImVec2(regionTL.x + 1, regionTL.y + 1),
        ImVec2(regionBR.x - 1, regionBR.y - 1), true);

    // 轻量 LOD：当每像素 <1 屏幕像素时，按步进聚合成块，代表色取左上像素
    const int stepX = std::max(1, (int)std::floor(1.0f / std::max(1e-6f, scale)));
    const int stepY = stepX;

    for (int y = 0; y < H; y += stepY) {
        for (int x = 0; x < W; x += stepX) {
            const int idx = y * W + x;
            const uint32_t v = pVTData[idx];
            const ImU32 col = VTReadbackDecodeData(v);

            ImVec2 tl = A2S((float)x, (float)y);
            ImVec2 br = A2S((float)std::min(x + stepX, W), (float)std::min(y + stepY, H));
            dl->AddRectFilled(tl, br, col);
        }
    }

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
    ImGui::DockBuilderDockWindow(m_strTitle[2].c_str(), dockspace_id);

    ImGui::DockBuilderFinish(dockspace_id);  // 完成！
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

    // 相机附近 Sector 高亮（来自 NXVirtualTexture）
    if (auto* pVT = GetVirtualTexture())
    {
        const auto& sectors = m_cachedData->sectors;

        for (const auto& sector : sectors)
        {
            ImU32 imageFill = GetSectorRectColor(sector.imageSize, true);
            ImU32 imageLine = GetSectorRectColor(sector.imageSize, false);

            // sector.id 是网格坐标，转换为世界坐标
            float minx = float(sector.id.x * SECTOR_SIZE);
            float maxx = float((sector.id.x + 1) * SECTOR_SIZE);
            float minz = float(sector.id.y * SECTOR_SIZE);
            float maxz = float((sector.id.y + 1) * SECTOR_SIZE);

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
            dl->AddRectFilled(tl, br, imageFill);

            // 像素对齐描边
            tl.x = AlignPx(tl.x); br.x = AlignPx(br.x);
            tl.y = AlignPx(tl.y); br.y = AlignPx(br.y);
            dl->AddRect(tl, br, imageLine, 0.0f, 0, 1.5f);
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

ImU32 NXGUIVirtualTexture::GetSectorRectColor(int size, bool isFill)
{
    const int alpha = isFill ? 70 : 220; // 中心区域相对透明一些

    switch (size)
    {
    case 256: return IM_COL32(255, 60, 60, alpha); // 红
    case 128: return IM_COL32(60, 255, 60, alpha); // 绿
    case  64: return IM_COL32(60, 160, 255, alpha); // 原先 256 的偏蓝
    case  32: return IM_COL32(60, 255, 255, alpha); // 青
    case  16: return IM_COL32(255, 60, 255, alpha); // 品红
    case   8: return IM_COL32(255, 160, 60, alpha); // 橙
    case   4: return IM_COL32(160, 60, 255, alpha); // 紫
    case   2: return IM_COL32(60, 255, 160, alpha); // 孔雀绿/蓝绿
    case   1: return IM_COL32(255, 255, 60, alpha); // 黄
    default:  return IM_COL32(255, 255, 255, alpha); // 默认白色
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
    // 世界坐标（x=列，y=行->Z）
    return ImVec2(float(WORLD_MIN + col * SECTOR_SIZE),float(WORLD_MIN + row * SECTOR_SIZE));
}

ImVec2 NXGUIVirtualTexture::TileMax(int row, int col) 
{
    ImVec2 mn = TileMin(row, col);
    return ImVec2(mn.x + SECTOR_SIZE, mn.y + SECTOR_SIZE);
}

ImU32 NXGUIVirtualTexture::VTReadbackDecodeData(int val) const
{
    const uint32_t r = val % 100u;
    const float t = (float)r / 99.0f;
    const uint8_t g = (uint8_t)ImClamp(t * 255.0f + 0.5f, 0.0f, 255.0f);
    return IM_COL32(g, g, g, 255);
}
