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

    // ��ȡ���� Pass / ��Դ
    std::vector<NXRGPassNodeBase*> passNodes = pRG->GetPassNodes();
    std::vector<NXRGResource*>     resources = pRG->GetResources();

    if (passNodes.empty() || resources.empty())
    {
        ImGui::Text("No pass nodes or resources.");
        ImGui::End();
        return;
    }

    // ��=Pass����=Resource
    int columnCount = 1 + static_cast<int>(passNodes.size());
    if (ImGui::BeginTable("RenderGraphTableFlipped", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        // ��ͷ
        ImGui::TableSetupColumn("Resource");

        // ÿ��Pass
        for (auto* pass : passNodes)
        {
            ImGui::TableSetupColumn(pass->GetName().c_str());
        }
        ImGui::TableHeadersRow();

        // ÿ��Resource
        for (auto* pRes : resources)
        {
            ImGui::TableNextRow();

            // ��һ�ж�Ӧ��Դ��
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(pRes->GetName().c_str());

            // ������ж�Ӧÿ��Pass
            for (int passIndex = 0; passIndex < passNodes.size(); ++passIndex)
            {
                ImGui::TableSetColumnIndex(passIndex + 1);

                auto* passNode = passNodes[passIndex];
                const auto& inputs = passNode->GetInputs();
                const auto& outputs = passNode->GetOutputs();

                // ��ȡ��Դ�����pass�Ķ�д״̬
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

                // ��ʾ
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
