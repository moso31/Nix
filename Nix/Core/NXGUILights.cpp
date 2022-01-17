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

        auto lightType = m_pCurrentLight->GetType();
        if (lightType == NXLight_Distant)
        {
            NXPBRDistantLight* pDistantLight = (NXPBRDistantLight*)m_pCurrentLight;

            float fDrugSpeedTransform = 0.01f;
            XMVECTOR dir = pDistantLight->GetDirection();
            if (ImGui::DragFloat3("Direction", dir.m128_f32, fDrugSpeedTransform))
            {
                pDistantLight->SetDirection(dir);
            }

            XMVECTOR color = pDistantLight->GetColor();
            if (ImGui::ColorEdit3("Color", color.m128_f32))
            {
                pDistantLight->SetColor(color);
            }

            float illuminance = pDistantLight->GetIlluminance();
            if (ImGui::DragFloat("Illuminance", &illuminance, fDrugSpeedTransform))
            {
                pDistantLight->SetIlluminance(illuminance);
            }
        }
    }

	ImGui::End();
}
