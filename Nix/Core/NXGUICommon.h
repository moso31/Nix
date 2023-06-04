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

	void RenderTextureIcon(ImTextureID ImTexID, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

	// 将字符串转换成GUIStyle
	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);

	// 将CBuffer转换成GUIStyle
	NXGUICBufferStyle	GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);

	// 获取GUIStyle对应的向量个数
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);

	// 获取GUIStyle对应的默认值
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

	std::string ConvertShaderResourceDataToNSLParam(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay);
}
