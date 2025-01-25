#include "NXGUIRenderGraph.h"

NXGUIRenderGraph::NXGUIRenderGraph(Renderer* pRenderer)
    : m_pRenderer(pRenderer)
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

    // 列=Pass，行=Resource
    int columnCount = 1 + static_cast<int>(passNodes.size());
    if (ImGui::BeginTable("RenderGraphTableFlipped", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        // 表头
        ImGui::TableSetupColumn("Resource");

        // 每个Pass
        for (auto* pass : passNodes)
        {
            ImGui::TableSetupColumn(pass->GetName().c_str());
        }
        ImGui::TableHeadersRow();

        // 每个Resource
        for (auto* pRes : resources)
        {
            ImGui::TableNextRow();

            // 第一列对应资源名
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(pRes->GetName().c_str());

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

    ImGui::End();
}
