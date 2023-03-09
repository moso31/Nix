#pragma once
#include <filesystem>

class NXMaterial;
namespace NXGUICommon
{
	void CreateDefaultMaterialFile(const std::filesystem::path& path, const std::string& matName);
	void UpdateMaterialFile(NXMaterial* pMaterial);
}
