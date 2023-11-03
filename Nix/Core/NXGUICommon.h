#pragma once
#include <filesystem>
#include "NXShaderDefinitions.h"

struct NXGUIAssetDragData
{
	std::filesystem::path srcPath;
};

class NXMaterial;
class NXGUIFileBrowser;
namespace NXGUICommon
{
	void RenderSmallTextureIcon(ImTextureID ImTexID, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

	// 将字符串转换成GUIStyle
	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);

	// 将CBuffer转换成GUIStyle
	NXGUICBufferStyle	GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);

	// 获取GUIStyle对应的向量个数
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);

	// 获取GUIStyle对应的默认值
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

	std::string ConvertShaderResourceDataToNSLParam(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay);

	// 生成新资产。用于ContentExplorer添加文件时确定 具体的序号。
	// 判断一下当前Folder下所有扩展名类型为 strSuffix 的文件，如果文件名是 strJudge + [任意数字] 的形式，记下这个数字。
	// 遍历完成时，确定 最大的那个数字+1。若没有这种文件，则使用 1。
	// 然后在当前文件夹下返回这个路径。
	std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge);
}
