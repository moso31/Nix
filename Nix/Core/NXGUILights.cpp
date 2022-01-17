#include "NXGUILights.h"
#include "NXScene.h"
#include "NXPBRLight.h"

NXGUILights::NXGUILights(NXScene* pScene) :
    m_pCurrentScene(pScene),
    m_pCurrentLight(nullptr),
    m_currentItemIdx(0)
{
}

void NXGUILights::Render()
{
	ImGui::Begin("Lights");

    if (ImGui::BeginListBox("LightList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
    {
        UINT i = 0;
        for (auto pLight : m_pCurrentScene->GetPBRLights())
        {
            const bool is_selected = (m_currentItemIdx == i);
            if (ImGui::Selectable(pLight->GetName().c_str(), is_selected))
            {
                m_currentItemIdx = i;
                m_pCurrentLight = pLight;
            }

            if (is_selected) ImGui::SetItemDefaultFocus();
            i++;
        }
        ImGui::EndListBox();
    }

    if (m_pCurrentLight)
    {
        std::string strName = m_pCurrentLight->GetName().c_str();
        if (ImGui::InputText("Name", &strName))
        {
            m_pCurrentLight->SetName(strName);
        }
    }

	ImGui::End();
}
