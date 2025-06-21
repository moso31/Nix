#include "NXGUIRenderGraph.h"

NXGUIRenderGraph::NXGUIRenderGraph(Renderer* pRenderer) : 
    m_pRenderer(pRenderer),
    m_pShowResource(nullptr)
{
}

void NXGUIRenderGraph::Render()
{
    ImGui::Begin("Render Graph");

    auto pRG = m_pRenderer->GetRenderGraph();
    if (!pRG)
    {
        ImGui::Text("Render Graph is not found.");
        ImGui::End();
        return;
    }

    // 获取所有 Pass / 资源
    std::vector<NXRGPassNodeBase*> passNodes = pRG->GetPassNodes();
    std::vector<NXRGResource*>     resources = pRG->GetResources();

    if (passNodes.empty() || resources.empty())
    {
        ImGui::Text("No pass nodes or resources.");
        ImGui::End();
        return;
    }

    ImVec2 totalSize = ImGui::GetContentRegionAvail();
    float tableWidth = totalSize.x * 0.7f;
    float imageWidth = totalSize.x - tableWidth;

    ImGui::BeginChild("TableRegion", ImVec2(tableWidth, 0), false);

    // 列=Pass，行=Resource
    int columnCount = 1 + static_cast<int>(passNodes.size());
    if (ImGui::BeginTable("RenderGraphTableFlipped", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        // 表头
        ImGui::TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthFixed, 200.0f);

        // 每个Pass
        for (auto* pass : passNodes)
        {
            ImGui::TableSetupColumn(pass->GetName().c_str(), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        }
        ImGui::TableHeadersRow();

        // 每个Resource
        for (auto* pRes : resources)
        {
            ImGui::TableNextRow();

            // 第一列对应资源名
            ImGui::TableSetColumnIndex(0);
            ImGui::SameLine();
            if (ImGui::SmallButton(pRes->GetName().c_str()))
            {
                m_pShowResource = pRes->GetResource();
            }

            // 后面的列对应每个Pass
            for (int passIndex = 0; passIndex < passNodes.size(); ++passIndex)
            {
                ImGui::TableSetColumnIndex(passIndex + 1);

                auto* passNode = passNodes[passIndex];
                const auto& inputs = passNode->GetInputs();
                const auto& outputs = passNode->GetOutputs();

                // 获取资源在这个pass的读写状态
                bool bRead = false;
                bool bWrite = false;
                for (auto& inSlot : inputs)
                {
                    if (inSlot.resource == pRes)
                    {
                        bRead = true;
                        break;
                    }
                }
                for (auto& outSlot : outputs)
                {
                    if (outSlot.resource == pRes)
                    {
                        bWrite = true;
                        break;
                    }
                }

                // 显示
                if (bRead && bWrite)
                {
                    ImGui::TextUnformatted("RW");
                }
                else if (bRead)
                {
                    ImGui::TextUnformatted("R");
                }
                else if (bWrite)
                {
                    ImGui::TextUnformatted("W");
                }
                else
                {
                    ImGui::TextUnformatted("-");
                }
            }
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ImageRegion", ImVec2(0, 0), false);
    if (m_pShowResource.IsValid())
    {
        if (m_pShowResource->GetResourceType() == NXResourceType::Buffer)
        {
            NXShVisDescHeap->PushFluid(m_pShowResource.As<NXBuffer>()->GetSRV());
        }
        else
        {
            NXShVisDescHeap->PushFluid(m_pShowResource.As<NXTexture>()->GetSRV());
        }

        auto& srvHandle = NXShVisDescHeap->Submit();
        const ImTextureID& ImTexID = (ImTextureID)srvHandle.ptr;
        ImGui::Image(ImTexID, ImVec2(300, 200));
    }
    ImGui::EndChild();

    ImGui::End();
}
