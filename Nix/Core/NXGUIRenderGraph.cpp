#include "NXGUIRenderGraph.h"
#include "NXRGResource.h"
#include "NXBuffer.h"
#include "NXTexture.h"
#include <vector>
#include <string>

NXGUIRenderGraph::NXGUIRenderGraph(Renderer* pRenderer) :
    m_pRenderer(pRenderer),
    m_pShowResource(nullptr)
{
}

void NXGUIRenderGraph::Render()
{
    auto pRenderGraph = m_pRenderer->GetRenderGraph();
    if (!pRenderGraph)
        return;

    // 从 RenderGraph 获取原始数据
    auto virtualResources = pRenderGraph->GetGUIVirtualResources();
    auto physicalResources = pRenderGraph->GetGUIPhysicalResources();
    auto importedResources = pRenderGraph->GetGUIImportedResources();
    int maxTimeLayer = pRenderGraph->GetMaxTimeLayer();

    ImGui::Begin("Render Graph");

    static int hoveredResource = -1;
    static int hoveredSegment = -1;
    static float paddingAxisX = 0.2f;
    static float rectSizeY = 0.4f;

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

    ImGui::Separator();

    // ==================== 概念资源 ====================
    if (m_viewMode == NXRGGUIResourceViewMode::Virtual)
    {
        hoveredResource = -1;

        // 根据复选框选择数据源
        auto displayResources = virtualResources;
        if (m_showImportedResources)
        {
            auto importedRes = pRenderGraph->GetGUIImportedResources();
            displayResources.insert(displayResources.end(), importedRes.begin(), importedRes.end());
        }
        virtualResources = displayResources;

        // 排序选项
        ImGui::Text(ImUtf8("排序方式:"));
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("按名称"), m_sortMode == NXRGGUISortMode::ByName))
            m_sortMode = NXRGGUISortMode::ByName;
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("按开始时间"), m_sortMode == NXRGGUISortMode::ByStartTime))
            m_sortMode = NXRGGUISortMode::ByStartTime;
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("按持续时间"), m_sortMode == NXRGGUISortMode::ByDuration))
            m_sortMode = NXRGGUISortMode::ByDuration;
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("无"), m_sortMode == NXRGGUISortMode::None))
            m_sortMode = NXRGGUISortMode::None;

        // GUI 层进行排序
        if (m_sortMode != NXRGGUISortMode::None)
        {
            std::sort(virtualResources.begin(), virtualResources.end(),
                [this](const NXRGGUIResource& a, const NXRGGUIResource& b) {
                    if (a.lifeTimes.empty() || b.lifeTimes.empty()) return false;
                    switch (m_sortMode)
                    {
                    case NXRGGUISortMode::ByName:
                        return a.name < b.name;
                    case NXRGGUISortMode::ByStartTime:
                        return a.lifeTimes[0].start < b.lifeTimes[0].start;
                    case NXRGGUISortMode::ByDuration:
                        return (a.lifeTimes[0].end - a.lifeTimes[0].start) > (b.lifeTimes[0].end - b.lifeTimes[0].start);
                    default:
                        return false;
                    }
                });
        }

        if (ImPlot::BeginPlot("##VirtualResourceLifetime", ImVec2(-1, 400), ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus))
        {
            ImPlot::SetupAxis(ImAxis_X1, ImUtf8("时间"), ImPlotAxisFlags_Opposite);  // X轴在顶部
            ImPlot::SetupAxis(ImAxis_Y1, ImUtf8("资源"), ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert);  // Y轴锁定并反转
            ImPlot::SetupAxisLimits(ImAxis_X1, -paddingAxisX, maxTimeLayer + paddingAxisX, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, (double)virtualResources.size() - 0.5, ImPlotCond_Always);
            ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");

            // 设置 Y 轴标签
            std::vector<const char*> labels;
            std::vector<double> positions;
            for (size_t i = 0; i < virtualResources.size(); i++)
            {
                labels.push_back(virtualResources[i].name.c_str());
                positions.push_back((double)i);
            }
            ImPlot::SetupAxisTicks(ImAxis_Y1, positions.data(), (int)positions.size(), labels.data());

            ImPlot::PushPlotClipRect(); // 超过表格的部分裁剪掉

            // 使用 ImPlot 的绘制列表来绘制自定义条形图
            ImDrawList* draw_list = ImPlot::GetPlotDrawList();

            for (size_t i = 0; i < virtualResources.size(); i++)
            {
                auto& res = virtualResources[i];
                if (res.lifeTimes.empty()) continue;

                auto& lifeTime = res.lifeTimes[0]; // 虚拟资源只有一个生命周期

                // 将时间坐标转换为屏幕坐标
                ImPlotPoint p1(lifeTime.start, (double)i - rectSizeY);
                ImPlotPoint p2(lifeTime.end, (double)i + rectSizeY);
                ImVec2 screen1 = ImPlot::PlotToPixels(p1);
                ImVec2 screen2 = ImPlot::PlotToPixels(p2);

                // 绘制矩形
                ImU32 color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
                draw_list->AddRectFilled(screen1, screen2, color);
                draw_list->AddRect(screen1, screen2, IM_COL32(0, 0, 0, 255));

                // 检测鼠标悬停
                if (ImPlot::IsPlotHovered())
                {
                    ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                    if (mouse.x >= lifeTime.start && mouse.x <= lifeTime.end &&
                        mouse.y >= (double)i - rectSizeY && mouse.y <= (double)i + rectSizeY)
                    {
                        hoveredResource = (int)i;

                        // 高亮显示
                        ImU32 highlight = IM_COL32(255, 255, 0, 100);
                        draw_list->AddRectFilled(screen1, screen2, highlight);
                        draw_list->AddRect(screen1, screen2, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                    }
                }
            }

            ImPlot::PopPlotClipRect();
            ImPlot::EndPlot();
        }

        // 悬停提示
        if (hoveredResource >= 0 && hoveredResource < (int)virtualResources.size())
        {
            auto& res = virtualResources[hoveredResource];
            if (!res.handles.empty())
            {
                auto pResource = pRenderGraph->GetResource(res.handles[0]);
                ImGui::BeginTooltip();
                ImGui::Text("Resource: %s", res.name.c_str());
                if (!res.lifeTimes.empty())
                {
                    ImGui::Text("Start: %d", res.lifeTimes[0].start);
                    ImGui::Text("End: %d", res.lifeTimes[0].end);
                    ImGui::Text("Duration: %d", res.lifeTimes[0].end - res.lifeTimes[0].start);
                }

                // 如果资源是纹理 显示纹理预览
                if (pResource->IsTexture())
                {
                    auto pTexture = pResource.As<NXTexture>();
                    if (pTexture.IsValid())
                    {
                        // 获取屏幕分辨率大小的1/10 作为图像宽度
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
                    // 如果资源是buffer 显示buffer信息
                    auto pBuffer = pResource.As<NXBuffer>();
                    if (pBuffer.IsValid())
                    {
                        // TODO 
                    }
                }

                ImGui::EndTooltip();
            }
        }
    }

    // ==================== 实际资源 ====================
    if (m_viewMode == NXRGGUIResourceViewMode::Physical)
    {
        hoveredResource = -1;
        hoveredSegment = -1;

        // 根据复选框选择数据源
        auto displayResources = physicalResources;
        if (m_showImportedResources)
        {
            auto importedRes = pRenderGraph->GetGUIImportedResources();
            displayResources.insert(displayResources.end(), importedRes.begin(), importedRes.end());
        }
        physicalResources = displayResources;

        // 排序选项
        ImGui::Text(ImUtf8("排序方式:"));
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("按名称##Physical"), m_sortMode == NXRGGUISortMode::ByName))
            m_sortMode = NXRGGUISortMode::ByName;
        ImGui::SameLine();
        if (ImGui::RadioButton(ImUtf8("无##Physical"), m_sortMode == NXRGGUISortMode::None))
            m_sortMode = NXRGGUISortMode::None;

        // GUI 层进行排序
        if (m_sortMode == NXRGGUISortMode::ByName)
        {
            std::sort(physicalResources.begin(), physicalResources.end(),
                [](const NXRGGUIResource& a, const NXRGGUIResource& b) {
                    return a.name < b.name;
                });
        }

        if (ImPlot::BeginPlot("##PhysicalResourceAllocation", ImVec2(-1, 400), ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus))
        {
            // 设置坐标轴
            ImPlot::SetupAxis(ImAxis_X1, ImUtf8("时间"), ImPlotAxisFlags_Opposite);  // X轴在顶部
            ImPlot::SetupAxis(ImAxis_Y1, ImUtf8("资源"), ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert);  // Y轴锁定并反转
            ImPlot::SetupAxisLimits(ImAxis_X1, -paddingAxisX, maxTimeLayer + paddingAxisX, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, (double)physicalResources.size() - 0.5, ImPlotCond_Always);
            ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");

            // 设置 Y 轴标签
            std::vector<const char*> labels;
            std::vector<double> positions;
            for (size_t i = 0; i < physicalResources.size(); i++)
            {
                labels.push_back(physicalResources[i].name.c_str());
                positions.push_back((double)i);
            }
            ImPlot::SetupAxisTicks(ImAxis_Y1, positions.data(), (int)positions.size(), labels.data());

            ImPlot::PushPlotClipRect(); // 超过表格的部分裁剪掉

            // 使用 ImPlot 的绘制列表
            ImDrawList* draw_list = ImPlot::GetPlotDrawList();

            for (size_t i = 0; i < physicalResources.size(); i++)
            {
                auto& res = physicalResources[i];

                // 绘制每个时间段
                for (size_t j = 0; j < res.lifeTimes.size(); j++)
                {
                    auto& lifeTime = res.lifeTimes[j];
                    int start = lifeTime.start;
                    int end = lifeTime.end;

                    // 将时间坐标转换为屏幕坐标
                    ImPlotPoint p1(start, (double)i - rectSizeY);
                    ImPlotPoint p2(end, (double)i + rectSizeY);
                    ImVec2 screen1 = ImPlot::PlotToPixels(p1);
                    ImVec2 screen2 = ImPlot::PlotToPixels(p2);

                    // 绘制矩形
                    ImU32 color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
                    draw_list->AddRectFilled(screen1, screen2, color);
                    draw_list->AddRect(screen1, screen2, IM_COL32(0, 0, 0, 255));

                    // 检测鼠标悬停
                    if (ImPlot::IsPlotHovered())
                    {
                        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                        if (mouse.x >= start && mouse.x <= end &&
                            mouse.y >= (double)i - rectSizeY && mouse.y <= (double)i + rectSizeY)
                        {
                            hoveredResource = (int)i;
                            hoveredSegment = (int)j;

                            // 高亮显示
                            ImU32 highlight = IM_COL32(255, 255, 0, 100);
                            draw_list->AddRectFilled(screen1, screen2, highlight);
                            draw_list->AddRect(screen1, screen2, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                        }
                    }
                }
            }

            ImPlot::PopPlotClipRect();
            ImPlot::EndPlot();
        }

        // 悬停提示
        if (hoveredResource >= 0 && hoveredResource < (int)physicalResources.size() && hoveredSegment >= 0)
        {
            auto& res = physicalResources[hoveredResource];
            if (hoveredSegment < (int)res.lifeTimes.size())
            {
                auto& lifeTime = res.lifeTimes[hoveredSegment];
                ImGui::BeginTooltip();
                ImGui::Text("Resource: %s", res.name.c_str());
                ImGui::Text("Segment: %d", hoveredSegment);
                ImGui::Text("Start: %d", lifeTime.start);
                ImGui::Text("End: %d", lifeTime.end);
                ImGui::Text("Duration: %d", lifeTime.end - lifeTime.start);
                ImGui::EndTooltip();
            }
        }
    }

    ImGui::End();
}
