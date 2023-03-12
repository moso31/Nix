#include "NXGUITexture.h"
#include <filesystem>
#include "NXResourceManager.h"

NXGUITexture::NXGUITexture()
{
}

void NXGUITexture::Render()
{
	ImGui::Begin("Texture");
	ImGui::End();
}

NXTexture2D* NXGUITexture::GetImage(const std::string& strTexPath)
{
	return nullptr;
}
