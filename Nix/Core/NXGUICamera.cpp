#include "BaseDefs/DearImGui.h"

#include "NXGUICamera.h"
#include "NXScene.h"
#include "NXCamera.h"

#include "NSFirstPersonalCamera.h" // 内置第一人称控制脚本

NXGUICamera::NXGUICamera(NXScene* pScene) :
    m_pCurrentScene(pScene),
    m_pCurrentCamera(pScene->GetMainCamera())
{
}

void NXGUICamera::Render()
{
	ImGui::Begin("Camera");

    if (m_pCurrentCamera)
    {
        std::string strName = m_pCurrentCamera->GetName().c_str();
        if (ImGui::InputText("Name", &strName))
        {
            m_pCurrentCamera->SetName(strName);
        }

        // 第一人称控制器
        NSFirstPersonalCamera* pFirstPersonalController = (NSFirstPersonalCamera*)m_pCurrentCamera->GetFirstPersonalController();
        if (pFirstPersonalController)
        {
            float value = pFirstPersonalController->GetMoveSpeed();
            if (ImGui::DragFloat("Move speed", &value, 0.01f, 0.0f, 100.0f))
            {
                pFirstPersonalController->SetMoveSpeed(value);
            }

            value = pFirstPersonalController->GetSensitivity();
            if (ImGui::DragFloat("Sensitivity", &value, 0.01f, 0.0f, 100.0f))
            {
                pFirstPersonalController->SetSensitivity(value);
            }
        }

        float fFovY = m_pCurrentCamera->GetFovY();
        if (ImGui::DragFloat("Fov", &fFovY, 0.1f, 0.0f, 180.0f))
        {
            m_pCurrentCamera->SetFovY(fFovY);
        }

        float fNear = m_pCurrentCamera->GetZNear();
        float fFar = m_pCurrentCamera->GetZFar();
        if (ImGui::DragFloat("ZNear", &fNear, 0.01f, 0.01f, fFar))
        {
            fNear = std::fmaxf(0.01f, fNear);
            m_pCurrentCamera->SetZNear(fNear);
        }

        if (ImGui::DragFloat("ZFar", &fFar, 0.01f, fNear, 10000.0f))
        {
            m_pCurrentCamera->SetZFar(fFar);
        }
    }

	ImGui::End();
}
