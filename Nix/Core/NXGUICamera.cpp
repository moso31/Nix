#include "NXGUICamera.h"
#include "NXScene.h"
#include "NXCamera.h"

#include "NSFirstPersonalCamera.h" // ���õ�һ�˳ƿ��ƽű�

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

        // ��һ�˳ƿ�����
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
    }

	ImGui::End();
}
