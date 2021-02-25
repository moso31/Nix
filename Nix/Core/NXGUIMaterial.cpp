#include "NXGUIMaterial.h"

NXGUIMaterial::NXGUIMaterial() :
	m_pCurrentScene(nullptr)
{
}

void NXGUIMaterial::Render()
{
	NXPrimitive* pPickingObject = static_cast<NXPrimitive*>(m_pCurrentScene->GetCurrentPickingObject());
	if (!pPickingObject)
		return;

	ImGui::Begin("Material");

	std::string strName = pPickingObject->GetName().c_str();
	ImGui::InputText("Name", &strName);

	XMVECTOR value = XMLoadFloat3(&pPickingObject->GetTranslation());
	ImGui::DragFloat3("Translation", value.m128_f32);

	value = XMLoadFloat3(&pPickingObject->GetRotation().EulerXYZ());
	ImGui::DragFloat3("Rotation", value.m128_f32);

	value = XMLoadFloat3(&pPickingObject->GetScale());
	ImGui::DragFloat3("Scale", value.m128_f32);

	NXPBRMaterial* pPickingObjectMaterial = pPickingObject->GetPBRMaterial();
	if (pPickingObjectMaterial)
	{
		static XMVECTOR value = { 1.0, 1.0, 1.0 };

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAlbedo());
		ImGui::SameLine();
		ImGui::ColorEdit3("Albedo", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVNormal());
		ImGui::SameLine();
		ImGui::ColorEdit3("Normal", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVMetallic());
		ImGui::SameLine();
		ImGui::DragFloat("Metallic", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVRoughness());
		ImGui::SameLine();
		ImGui::DragFloat("Roughness", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAO());
		ImGui::SameLine();
		ImGui::DragFloat("AO", value.m128_f32);
	}

	ImGui::End();
}
