#include "NXGUITexture.h"
#include <filesystem>
#include "NXResourceManager.h"

NXGUITexture::NXGUITexture()
{
}

void NXGUITexture::Render()
{
	ImGui::Begin("Texture");

	//NXResourceManager::CreateTexture2D

	ImGui::Button("hello", ImVec2(200.0f, 200.0f));

	bool x;
	ImGui::Checkbox("Generate mip map", &x);
	ImGui::Checkbox("Invert normal Y", &x);

	int iCurrItem = 0;
	static const char* items[] = { "Default", "Normal map" };
	ImGui::Combo("Texture type", &iCurrItem, items, IM_ARRAYSIZE(items));

	if (iCurrItem == 1)
	{
		ImGui::Checkbox("sRGB", &x);
	}

	ImGui::End();
}
