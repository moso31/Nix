#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"

struct NXGUIAssetDragData
{
	std::filesystem::path srcPath;
};

class NXMaterial;
class NXGUIFileBrowser;
namespace NXGUICommon
{
	void CreateDefaultMaterialFile(const std::filesystem::path& path, const std::string& matName);
	void SaveMaterialFile(NXMaterial* pMaterial);

	void RenderTextureIcon(ImTextureID ImTexID, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);
	NXGUICBufferStyle	GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);
}
