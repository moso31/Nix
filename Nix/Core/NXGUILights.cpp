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

            XMVECTOR dir = pDistantLight->GetDirection();
            if (ImGui::DragFloat3("Direction", dir.m128_f32, 0.01f))
            {
                pDistantLight->SetDirection(dir);
            }

            XMVECTOR color = pDistantLight->GetColor();
            if (ImGui::ColorEdit3("Color", color.m128_f32))
            {
                pDistantLight->SetColor(color);
            }

            float illuminance = pDistantLight->GetIlluminance();
            if (ImGui::DragFloat("Illuminance", &illuminance, 0.01f))
            {
                pDistantLight->SetIlluminance(illuminance);
            }
        }

        if (lightType == NXLight_Point)
        {
            NXPBRPointLight* pPointLight = (NXPBRPointLight*)m_pCurrentLight;

            XMVECTOR pos = pPointLight->GetPosition();
            if (ImGui::DragFloat3("position", pos.m128_f32, 0.01f))
            {
                pPointLight->SetPosition(pos);
            }

            XMVECTOR color = pPointLight->GetColor();
            if (ImGui::ColorEdit3("Color", color.m128_f32))
            {
                pPointLight->SetColor(color);
            }

            float intensity = pPointLight->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.01f))
            {
                pPointLight->SetIntensity(intensity);
            }

            float influenceRadius = pPointLight->GetInfluenceradius();
            if (ImGui::DragFloat("Influence Radius", &influenceRadius, 0.1f, 0.0f, FLT_MAX))
            {
                pPointLight->SetInfluenceRadius(influenceRadius);
            }
        }

        if (lightType == NXLight_Spot)
        {
            NXPBRSpotLight* pSpotLight = (NXPBRSpotLight*)m_pCurrentLight;

            XMVECTOR pos = pSpotLight->GetPosition();
            if (ImGui::DragFloat3("position", pos.m128_f32, 0.01f))
            {
                pSpotLight->SetPosition(pos);
            }

            XMVECTOR dir = pSpotLight->GetDirection();
            if (ImGui::DragFloat3("Direction", dir.m128_f32, 0.01f))
            {
                pSpotLight->SetDirection(dir);
            }

            XMVECTOR color = pSpotLight->GetColor();
            if (ImGui::ColorEdit3("Color", color.m128_f32))
            {
                pSpotLight->SetColor(color);
            }

            float illuminance = pSpotLight->GetIntensity();
            if (ImGui::DragFloat("Intensity", &illuminance, 0.01f))
            {
                pSpotLight->SetIntensity(illuminance);
            }

            float innerAngle = pSpotLight->GetInnerAngle();
            float outerAngle = pSpotLight->GetOuterAngle();
            if (ImGui::DragFloat("Inner Angle", &innerAngle, 1.0f, 0.0f, outerAngle))
            {
                pSpotLight->SetInnerAngle(innerAngle);
            }

            if (ImGui::DragFloat("Outer Angle", &outerAngle, 1.0f, innerAngle, 180.0f))
            {
                pSpotLight->SetOuterAngle(outerAngle);
            }

            float influenceRadius = pSpotLight->GetInfluenceradius();
            if (ImGui::DragFloat("Influence Radius", &influenceRadius, 0.1f, 0.0f, FLT_MAX))
            {
                pSpotLight->SetInfluenceRadius(influenceRadius);
            }
        }
    }

	ImGui::End();
}
