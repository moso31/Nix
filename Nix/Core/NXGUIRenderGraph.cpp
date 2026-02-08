#include "NXGUIRenderGraph.h"
#include "NXRGResource.h"
#include "NXBuffer.h"
#include "NXTexture.h"
#include <vector>
#include <string>

// ==================== 静态辅助方法实现 ====================

constexpr ImVec4 NXGUIRenderGraph::ImU32ToImVec4(ImU32 color)
{
    return ImVec4(
        ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
    );
}

std::string NXGUIRenderGraph::FormatByteSize(size_t bytes)
    {
        char buffer[64];
        if (bytes < 1024)
        {
            sprintf_s(buffer, "%zu B", bytes);
        }
        else if (bytes < 1024 * 1024)
        {
            sprintf_s(buffer, "%.2f KB", bytes / 1024.0);
        }
        else if (bytes < 1024 * 1024 * 1024)
        {
            sprintf_s(buffer, "%.2f MB", bytes / (1024.0 * 1024.0));
        }
        else
        {
            sprintf_s(buffer, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
        }
        return std::string(buffer);
    }

const char* NXGUIRenderGraph::GetDXGIFormatString(int format)
{
        switch (format)
        {
        case 0: return "UNKNOWN";
        case 1: return "R32G32B32A32_TYPELESS";
        case 2: return "R32G32B32A32_FLOAT";
        case 3: return "R32G32B32A32_UINT";
        case 4: return "R32G32B32A32_SINT";
        case 5: return "R32G32B32_TYPELESS";
        case 6: return "R32G32B32_FLOAT";
        case 7: return "R32G32B32_UINT";
        case 8: return "R32G32B32_SINT";
        case 9: return "R16G16B16A16_TYPELESS";
        case 10: return "R16G16B16A16_FLOAT";
        case 11: return "R16G16B16A16_UNORM";
        case 12: return "R16G16B16A16_UINT";
        case 13: return "R16G16B16A16_SNORM";
        case 14: return "R16G16B16A16_SINT";
        case 15: return "R32G32_TYPELESS";
        case 16: return "R32G32_FLOAT";
        case 17: return "R32G32_UINT";
        case 18: return "R32G32_SINT";
        case 19: return "R32G8X24_TYPELESS";
        case 20: return "D32_FLOAT_S8X24_UINT";
        case 21: return "R32_FLOAT_X8X24_TYPELESS";
        case 22: return "X32_TYPELESS_G8X24_UINT";
        case 23: return "R10G10B10A2_TYPELESS";
        case 24: return "R10G10B10A2_UNORM";
        case 25: return "R10G10B10A2_UINT";
        case 26: return "R11G11B10_FLOAT";
        case 27: return "R8G8B8A8_TYPELESS";
        case 28: return "R8G8B8A8_UNORM";
        case 29: return "R8G8B8A8_UNORM_SRGB";
        case 30: return "R8G8B8A8_UINT";
        case 31: return "R8G8B8A8_SNORM";
        case 32: return "R8G8B8A8_SINT";
        case 33: return "R16G16_TYPELESS";
        case 34: return "R16G16_FLOAT";
        case 35: return "R16G16_UNORM";
        case 36: return "R16G16_UINT";
        case 37: return "R16G16_SNORM";
        case 38: return "R16G16_SINT";
        case 39: return "R32_TYPELESS";
        case 40: return "D32_FLOAT";
        case 41: return "R32_FLOAT";
        case 42: return "R32_UINT";
        case 43: return "R32_SINT";
        case 44: return "R24G8_TYPELESS";
        case 45: return "D24_UNORM_S8_UINT";
        case 46: return "R24_UNORM_X8_TYPELESS";
        case 47: return "X24_TYPELESS_G8_UINT";
        case 48: return "R8G8_TYPELESS";
        case 49: return "R8G8_UNORM";
        case 50: return "R8G8_UINT";
        case 51: return "R8G8_SNORM";
        case 52: return "R8G8_SINT";
        case 53: return "R16_TYPELESS";
        case 54: return "R16_FLOAT";
        case 55: return "D16_UNORM";
        case 56: return "R16_UNORM";
        case 57: return "R16_UINT";
        case 58: return "R16_SNORM";
        case 59: return "R16_SINT";
        case 60: return "R8_TYPELESS";
        case 61: return "R8_UNORM";
        case 62: return "R8_UINT";
        case 63: return "R8_SNORM";
        case 64: return "R8_SINT";
        case 65: return "A8_UNORM";
        case 66: return "R1_UNORM";
        case 67: return "R9G9B9E5_SHAREDEXP";
        case 68: return "R8G8_B8G8_UNORM";
        case 69: return "G8R8_G8B8_UNORM";
        case 70: return "BC1_TYPELESS";
        case 71: return "BC1_UNORM";
        case 72: return "BC1_UNORM_SRGB";
        case 73: return "BC2_TYPELESS";
        case 74: return "BC2_UNORM";
        case 75: return "BC2_UNORM_SRGB";
        case 76: return "BC3_TYPELESS";
        case 77: return "BC3_UNORM";
        case 78: return "BC3_UNORM_SRGB";
        case 79: return "BC4_TYPELESS";
        case 80: return "BC4_UNORM";
        case 81: return "BC4_SNORM";
        case 82: return "BC5_TYPELESS";
        case 83: return "BC5_UNORM";
        case 84: return "BC5_SNORM";
        case 85: return "B5G6R5_UNORM";
        case 86: return "B5G5R5A1_UNORM";
        case 87: return "B8G8R8A8_UNORM";
        case 88: return "B8G8R8X8_UNORM";
        case 89: return "R10G10B10_XR_BIAS_A2_UNORM";
        case 90: return "B8G8R8A8_TYPELESS";
        case 91: return "B8G8R8A8_UNORM_SRGB";
        case 92: return "B8G8R8X8_TYPELESS";
        case 93: return "B8G8R8X8_UNORM_SRGB";
        case 94: return "BC6H_TYPELESS";
        case 95: return "BC6H_UF16";
        case 96: return "BC6H_SF16";
        case 97: return "BC7_TYPELESS";
        case 98: return "BC7_UNORM";
        case 99: return "BC7_UNORM_SRGB";
        case 100: return "AYUV";
        case 101: return "Y410";
        case 102: return "Y416";
        case 103: return "NV12";
        case 104: return "P010";
        case 105: return "P016";
        case 106: return "420_OPAQUE";
        case 107: return "YUY2";
        case 108: return "Y210";
        case 109: return "Y216";
        case 110: return "NV11";
        case 111: return "AI44";
        case 112: return "IA44";
        case 113: return "P8";
        case 114: return "A8P8";
        case 115: return "B4G4R4A4_UNORM";
        case 130: return "P208";
        case 131: return "V208";
        case 132: return "V408";
        case 189: return "SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
        case 190: return "SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";
        case 191: return "A4B4G4R4_UNORM";
        default: return "UNKNOWN";
        }
    }

void NXGUIRenderGraph::ShowResourceTooltip(const NXRGGUIResource& res, NXRenderGraph* pRenderGraph, int segmentIndex)
{
        if (res.handles.empty()) return;

        auto pResource = pRenderGraph->GetResource(res.handles[segmentIndex]);
        if (pResource.IsNull()) return;

        ImGui::BeginTooltip();

        // 显示生命周期信息
        if (segmentIndex >= 0 && segmentIndex < (int)res.lifeTimes.size())
        {
            auto& lifeTime = res.lifeTimes[segmentIndex];
            auto pStartPass = lifeTime.pStartPass;
            auto pEndPass = lifeTime.pEndPass;

            if (!res.isImported)
            {
                auto pRGResource = pRenderGraph->GetResourceMap();
                ImGui::Text("Virtual Name: %s", pRGResource[res.handles[segmentIndex]]->GetName().c_str());
            }

            if (pStartPass)
                ImGui::Text("First Used In Pass: %s", pStartPass->GetName().c_str());
            if (pEndPass)
                ImGui::Text("Last Used In Pass: %s", pEndPass->GetName().c_str());
        }

        // 如果资源是纹理，显示纹理信息和预览
        if (pResource->IsTexture())
        {
            auto pTexture = pResource.As<NXTexture>();
            if (pTexture.IsValid())
            {
                ImGui::Separator();
                ImGui::Text("Type: Texture");
                ImGui::Text("Format: %s", GetDXGIFormatString(pTexture->GetFormat()));
                ImGui::Text("Size: %d x %d", pTexture->GetWidth(), pTexture->GetHeight());
                ImGui::Text("Array Size: %d", pTexture->GetArraySize());
                ImGui::Text("Mip Levels: %d", pTexture->GetMipLevels());
                if (res.isImported)
                {
					ImGui::Text("File: %s", pTexture->GetFilePath().filename().string().c_str());
                }

                // 显示纹理预览
                float fW = std::max(ImGui::GetIO().DisplaySize.x * 0.2f, 200.0f);
                float fH = (float)pTexture->GetHeight() * (fW / (float)pTexture->GetWidth());

                ImGui::Separator();
                NXShVisDescHeap->PushFluid(pTexture->GetSRVPreview(0));
                auto srvHandle = NXShVisDescHeap->Submit();
                ImTextureID ImTexID = (ImTextureID)srvHandle.ptr;
                ImGui::Image(ImTexID, ImVec2(fW, fH));
            }
        }
        else if (pResource->IsBuffer())
        {
            auto pBuffer = pResource.As<NXBuffer>();
            if (pBuffer.IsValid())
            {
                ImGui::Separator();
                ImGui::Text("Type: Buffer");
				ImGui::Text("Size: %s", FormatByteSize(pBuffer->GetByteSize()).c_str());
				ImGui::Text("Stride: %s", FormatByteSize(pBuffer->GetStride()).c_str());
				ImGui::Text("Element Count: %d", pBuffer->GetWidth());
            }
        }

        ImGui::EndTooltip();
    }

// 1. 只绘制高亮矩形（在裁剪区域内调用）
void NXGUIRenderGraph::DrawTimeLayerHighlightRect(int timeLayer, int minTimeLayer, int maxTimeLayer, double resourceCount, float bottomPadding)
{
        if (!ImPlot::IsPlotHovered())
            return;

        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        int currentTimeLayer = (int)std::round(mouse.x);
        if (currentTimeLayer != timeLayer || currentTimeLayer < minTimeLayer || currentTimeLayer > maxTimeLayer)
            return;

        // 绘制高亮竖线（从顶部到底部，包含底部padding区域）
        ImPlotPoint p1(currentTimeLayer, -0.5);
        ImPlotPoint p2(currentTimeLayer, resourceCount - 0.5 + bottomPadding);
        ImVec2 screen1 = ImPlot::PlotToPixels(p1);
        ImVec2 screen2 = ImPlot::PlotToPixels(p2);
        ImPlot::GetPlotDrawList()->AddLine(screen1, screen2, COLOR_HIGHLIGHT_BORDER, 2.0f);
    }

// 2. 绘制Pass名称文本（在裁剪区域外调用）
void NXGUIRenderGraph::DrawTimeLayerPassNames(int timeLayer, int minTimeLayer, int maxTimeLayer, double resourceCount, NXRenderGraph* pRenderGraph)
{
        if (!ImPlot::IsPlotHovered())
            return;

        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        int currentTimeLayer = (int)std::round(mouse.x);
        if (currentTimeLayer != timeLayer || currentTimeLayer < minTimeLayer || currentTimeLayer > maxTimeLayer)
            return;

        const auto& passes = pRenderGraph->GetPassesAtTimeLayer(currentTimeLayer);
        if (passes.empty())
            return;

        // 使用 ImGui 的 WindowDrawList，不受 Plot 裁剪限制
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        int lineIndex = 0;
        for (const auto& passName : passes)
        {
            ImVec2 textSize = ImGui::CalcTextSize(passName.c_str());

            ImPlotPoint textPlotPos(currentTimeLayer, -lineIndex);
            ImVec2 textScreenPos = ImPlot::PlotToPixels(textPlotPos);

            float textX = textScreenPos.x;
            ImVec2 textPos(textX - textSize.x * 0.5f, textScreenPos.y - textSize.y * 0.5f);

            // 绘制背景
            ImVec2 bgMin(textPos.x - 2.0f, textPos.y - 1.0f);
            ImVec2 bgMax(textPos.x + textSize.x + 2.0f, textPos.y + textSize.y + 1.0f);
            draw_list->AddRectFilled(bgMin, bgMax, COLOR_TEXT_BACKGROUND);

            // 绘制文字
            draw_list->AddText(textPos, COLOR_TEXT_FOREGROUND, passName.c_str());
            lineIndex++;
        }
    }

// 3. 绘制非导入资源和导入资源之间的分隔线
void NXGUIRenderGraph::DrawResourceSeparatorLine(size_t nonImportedCount, int minTimeLayer, int maxTimeLayer)
{
        if (nonImportedCount == 0) return;

        // 在非导入资源的最后一个和导入资源的第一个之间画线
        double separatorY = (double)nonImportedCount - 0.5;

        ImPlotPoint p1(minTimeLayer - 0.5, separatorY);
        ImPlotPoint p2(maxTimeLayer + 0.5, separatorY);

        ImVec2 screen1 = ImPlot::PlotToPixels(p1);
        ImVec2 screen2 = ImPlot::PlotToPixels(p2);

    // 绘制白色分隔线
    ImPlot::GetPlotDrawList()->AddLine(screen1, screen2, COLOR_SEPARATOR_LINE, 1.0f);
}

void NXGUIRenderGraph::RenderResourceView(
    const std::vector<NXRGGUIResource>& displayResources,
    NXRenderGraph* pRenderGraph,
    int minTimeLayer,
    int maxTimeLayer,
    const char* plotLabel,
    bool isPhysicalMode)
{
    static int hoveredResource = -1;
    static int hoveredSegment = -1;
    static int hoveredTimeLayer = -1;
    static float paddingAxisX = 0.5f;
    static float rectSizeY = 0.4f;
    static float bottomPadding = 0.0f;
    static float importedResourceOffsetX = 0.5f;

    // 重置hover状态
    hoveredResource = -1;
    hoveredTimeLayer = -1;
    if (isPhysicalMode)
    {
        hoveredSegment = -1;
    }

    // 计算非导入资源数量
    size_t nonImportedCount = 0;
    for (const auto& res : displayResources)
    {
        if (!res.isImported)
            nonImportedCount++;
    }

    if (ImPlot::BeginPlot(plotLabel, ImVec2(-1, -1), ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect))
    {
        ImPlot::SetupAxis(ImAxis_X1, ImUtf8(""), ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxis(ImAxis_Y1, ImUtf8("资源"), ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert);
        ImPlot::SetupAxisLimits(ImAxis_X1, minTimeLayer - paddingAxisX, maxTimeLayer + paddingAxisX, ImPlotCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, (double)displayResources.size() - 0.5 + bottomPadding, ImPlotCond_Always);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, minTimeLayer - paddingAxisX, maxTimeLayer + paddingAxisX);
        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");

        // 设置 Y 轴标签
        std::vector<const char*> labels;
        std::vector<double> positions;
        for (size_t i = 0; i < displayResources.size(); i++)
        {
            labels.push_back(displayResources[i].name.c_str());
            positions.push_back((double)i);
        }
        ImPlot::SetupAxisTicks(ImAxis_Y1, positions.data(), (int)positions.size(), labels.data());

        ImPlot::PushPlotClipRect();

        ImDrawList* draw_list = ImPlot::GetPlotDrawList();

        // 如果鼠标悬停在某个时间层，先绘制列高亮
        if (ImPlot::IsPlotHovered())
        {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            int currentTimeLayer = (int)std::round(mouse.x);
            DrawTimeLayerHighlightRect(currentTimeLayer, minTimeLayer, maxTimeLayer, (double)displayResources.size(), bottomPadding);
        }

        for (size_t i = 0; i < displayResources.size(); i++)
        {
            auto& res = displayResources[i];
            
            // Virtual模式下跳过空生命周期
            if (!isPhysicalMode && res.lifeTimes.empty()) 
                continue;

            // 绘制每个时间段
            for (size_t j = 0; j < res.lifeTimes.size(); j++)
            {
                auto& lifeTime = res.lifeTimes[j];

                ImU32 color = res.isImported ? COLOR_IMPORTED_RESOURCE : COLOR_REGULAR_RESOURCE;
                
                if (res.isImported)
                {
                    // 导入资源绘制为圆形
                    double centerX = (lifeTime.start + lifeTime.end) * 0.5 - importedResourceOffsetX;
                    double centerY = (double)i;

                    ImPlotPoint centerPlot(centerX, centerY);
                    ImVec2 centerScreen = ImPlot::PlotToPixels(centerPlot);

                    ImPlotPoint radiusTestPlot(centerX, centerY + rectSizeY);
                    ImVec2 radiusTestScreen = ImPlot::PlotToPixels(radiusTestPlot);
                    float radius = std::abs(radiusTestScreen.y - centerScreen.y);

                    draw_list->AddCircleFilled(centerScreen, radius, color);

                    // hover检测
                    if (ImPlot::IsPlotHovered())
                    {
                        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                        double dx = mouse.x - centerX;
                        double dy = mouse.y - centerY;
                        if (dx * dx + dy * dy <= rectSizeY * rectSizeY)
                        {
                            hoveredResource = (int)i;
                            if (isPhysicalMode)
                                hoveredSegment = (int)j;
                            hoveredTimeLayer = (int)std::round(mouse.x);

                            draw_list->AddCircleFilled(centerScreen, radius, COLOR_HIGHLIGHT_FILL);
                            draw_list->AddCircle(centerScreen, radius, COLOR_HIGHLIGHT_BORDER, 0, 2.0f);
                        }
                    }
                }
                else
                {
                    // 常规资源绘制为矩形
                    ImPlotPoint p1(lifeTime.start, (double)i - rectSizeY);
                    ImPlotPoint p2(lifeTime.end, (double)i + rectSizeY);

                    ImVec2 screen1 = ImPlot::PlotToPixels(p1);
                    ImVec2 screen2 = ImPlot::PlotToPixels(p2);

                    draw_list->AddRectFilled(screen1, screen2, color);

                    // 检测鼠标悬停
                    if (ImPlot::IsPlotHovered())
                    {
                        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                        float offset = res.isImported ? -importedResourceOffsetX : 0.0f;
                        if (mouse.x >= lifeTime.start + offset && mouse.x <= lifeTime.end + offset &&
                            mouse.y >= (double)i - rectSizeY && mouse.y <= (double)i + rectSizeY)
                        {
                            hoveredResource = (int)i;
                            if (isPhysicalMode)
                                hoveredSegment = (int)j;
                            hoveredTimeLayer = (int)std::round(mouse.x);

                            draw_list->AddRectFilled(screen1, screen2, COLOR_HIGHLIGHT_FILL);
                            draw_list->AddRect(screen1, screen2, COLOR_HIGHLIGHT_BORDER, 0.0f, 0, 2.0f);
                        }
                        else if (mouse.x >= lifeTime.start + offset && mouse.x <= lifeTime.end + offset)
                        {
                            hoveredTimeLayer = (int)std::round(mouse.x);
                        }
                    }
                }
            }
        }

        // 绘制分隔线（如果有导入资源）
        if (nonImportedCount > 0 && nonImportedCount < displayResources.size())
        {
            DrawResourceSeparatorLine(nonImportedCount, minTimeLayer, maxTimeLayer);
        }

        ImPlot::PopPlotClipRect();

        // 绘制Pass名称（在裁剪区域外）
        if (ImPlot::IsPlotHovered())
        {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            int currentTimeLayer = (int)std::round(mouse.x);
            DrawTimeLayerPassNames(currentTimeLayer, minTimeLayer, maxTimeLayer, (double)displayResources.size(), pRenderGraph);
        }

        ImPlot::EndPlot();
    }

    // 悬停提示
    if (hoveredResource >= 0 && hoveredResource < (int)displayResources.size())
    {
        int segmentIndex = isPhysicalMode ? hoveredSegment : 0;
        if (!isPhysicalMode || (isPhysicalMode && hoveredSegment >= 0))
        {
            ShowResourceTooltip(displayResources[hoveredResource], pRenderGraph, segmentIndex);
        }
    }
}

NXGUIRenderGraph::NXGUIRenderGraph(Renderer* pRenderer) :
    m_pRenderer(pRenderer),
    m_pShowResource(nullptr)
{
}

void NXGUIRenderGraph::Render()
{
    // 如果窗口关闭，直接返回，避免不必要的渲染开销
    if (!m_bShowWindow)
        return;

    auto pRenderGraph = m_pRenderer->GetRenderGraph();
    if (!pRenderGraph)
        return;

    // 从 RenderGraph 获取原始数据
    auto virtualResources = pRenderGraph->GetGUIVirtualResources();
    auto physicalResources = pRenderGraph->GetGUIPhysicalResources();
    auto importedResources = pRenderGraph->GetGUIImportedResources();
    int minTimeLayer = m_showImportedResources ? 0 : pRenderGraph->GetMinTimeLayer();
    int maxTimeLayer = pRenderGraph->GetMaxTimeLayer();

    ImGui::Begin("Render Graph", &m_bShowWindow);

    static int hoveredResource = -1;
    static int hoveredSegment = -1;
    static int hoveredTimeLayer = -1;  // 用于记录鼠标悬停的时间层
    static float paddingAxisX = 0.5f;
    static float rectSizeY = 0.4f;
    static float bottomPadding = 0.0f;  // 底部固定预留空间，用于显示Pass名称
    static float importedResourceOffsetX = 0.5f;  // 导入资源的X轴偏移量

    // 使用下拉菜单选择显示模式
    const char* viewModes[] = { ImUtf8("虚拟资源"), ImUtf8("物理资源") };
    int currentMode = (int)m_viewMode;
    float comboWidth = std::max(ImGui::GetContentRegionAvail().x * 0.5f, 400.0f);
    ImGui::SetNextItemWidth(comboWidth);
    if (ImGui::Combo(ImUtf8("资源视图"), &currentMode, viewModes, IM_ARRAYSIZE(viewModes)))
    {
        m_viewMode = (NXRGGUIResourceViewMode)currentMode;
    }
    ImGui::SameLine();
    ImGui::Checkbox(ImUtf8("显示导入资源"), &m_showImportedResources);

    // 添加帮助按钮
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(ImUtf8("渲染图资源生命周期可视化说明:\n\n"));
        ImGui::BulletText(ImUtf8("横轴: 渲染Pass的执行顺序(时间层)"));
        ImGui::BulletText(ImUtf8("纵轴: 各个资源"));

        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextColored(ImU32ToImVec4(COLOR_REGULAR_RESOURCE), ImUtf8("蓝色条"));
        ImGui::SameLine();
        ImGui::Text(ImUtf8(": 常规资源的生命周期,表示该资源介创建到销毁的时间范围"));

        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextColored(ImU32ToImVec4(COLOR_IMPORTED_RESOURCE), ImUtf8("草绿色条"));
        ImGui::SameLine();
        ImGui::Text(ImUtf8(": 导入资源,这些是外部提供的资源"));

        // 对"悬停在资源条上"加框
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::Text(ImUtf8("将鼠标"));
        ImGui::SameLine(0, 0);
        ImVec2 textPos1 = ImGui::GetCursorScreenPos();
        ImVec2 textSize1 = ImGui::CalcTextSize(ImUtf8("悬停在资源条上"));
        ImGui::GetWindowDrawList()->AddRect(
            ImVec2(textPos1.x - 2, textPos1.y - 1),
            ImVec2(textPos1.x + textSize1.x + 2, textPos1.y + textSize1.y + 1),
            IM_COL32(255, 255, 0, 255), // 黄色框
            2.0f, 0, 1.5f
        );
        ImGui::Text(ImUtf8("悬停在资源条上"));
        ImGui::SameLine(0, 0);
        ImGui::Text(ImUtf8("可查看详细信息"));

        // 对"悬停在时间层上"加底色(和黄色竖条相同颜色)
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::Text(ImUtf8("将鼠标"));
        ImGui::SameLine(0, 0);
        ImVec2 textPos2 = ImGui::GetCursorScreenPos();
        ImVec2 textSize2 = ImGui::CalcTextSize(ImUtf8("悬停在时间层上"));
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(textPos2.x - 2, textPos2.y - 1),
            ImVec2(textPos2.x + textSize2.x + 2, textPos2.y + textSize2.y + 1),
            COLOR_COLUMN_HIGHLIGHT // 黄色半透明底色,和竖条颜色相同
        );
        ImGui::Text(ImUtf8("悬停在时间层上"));
        ImGui::SameLine(0, 0);
        ImGui::Text(ImUtf8("可查看该层执行的Pass"));

        ImGui::BulletText(ImUtf8("白色分隔线: 区分常规资源和导入资源"));
        ImGui::Separator();
        ImGui::TextUnformatted(ImUtf8("视图模式:\n"));
        ImGui::BulletText(ImUtf8("虚拟资源: 显示逻辑资源的生命周期"));
        ImGui::BulletText(ImUtf8("物理资源: 显示实际分配的资源,可能会有多个时间段(资源复用)"));
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    ImGui::Separator();

    // ==================== 虚拟资源 ====================
    if (m_viewMode == NXRGGUIResourceViewMode::Virtual)
    {
        // 根据复选框选择数据源
        auto displayResources = virtualResources;

        // 对Create类型的资源排序
        std::sort(displayResources.begin(), displayResources.end(),
            [](const NXRGGUIResource& a, const NXRGGUIResource& b) {
                if (a.lifeTimes.empty() || b.lifeTimes.empty()) return false;
                return a.lifeTimes[0].start < b.lifeTimes[0].start;
            });

        if (m_showImportedResources)
        {
            displayResources.insert(displayResources.end(), importedResources.begin(), importedResources.end());
        }

        RenderResourceView(displayResources, pRenderGraph, minTimeLayer, maxTimeLayer, "##VirtualResourceLifetime", false);
    }

    // ==================== 实际资源 ====================
    if (m_viewMode == NXRGGUIResourceViewMode::Physical)
    {
        // 根据复选框选择数据源
        auto displayResources = physicalResources;

        // 对Create类型的资源排序
        std::sort(displayResources.begin(), displayResources.end(),
            [](const NXRGGUIResource& a, const NXRGGUIResource& b) {
                return a.lifeTimes[0].start < b.lifeTimes[0].start;
            });

        if (m_showImportedResources)
        {
            displayResources.insert(displayResources.end(), importedResources.begin(), importedResources.end());
        }

        RenderResourceView(displayResources, pRenderGraph, minTimeLayer, maxTimeLayer, "##PhysicalResourceAllocation", true);
    }

    ImGui::End();
}
