#pragma once
#include <filesystem>
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

struct NXGUIAssetDragData
{
	std::filesystem::path srcPath;
};

enum class NXGUISmallTexType
{
	Image = 1,
	Raw = 1 << 1,
	All = Image | Raw
};

class NXGUIFileBrowser;
namespace NXGUICommon
{
	void RenderSmallTextureIcon(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop, NXGUISmallTexType type = NXGUISmallTexType::Image);

	// 将字符串转换成GUIStyle
	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);

	// 获取GUIStyle对应的向量个数
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);

	// 获取GUIStyle对应的默认值
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

	// 用于生成新资产时，ContentExplorer添加文件时确定 具体的序号
	// 比如某个文件夹下新建了材质，应该叫 "New Material ?"，这个方法就用于确定 ? 是多少。
	std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge);

	// 遍历指定文件夹下的所有文件（注意是递归遍历），并返回一个所有扩展名为 strSuffix 的文件数组。
	// 2023.11.4 此方法，目前主要是材质编辑器中，替换 nssprof 的时候，对应的替换popup使用。将来换纹理，换材质等，也会基于这个方法
	std::vector<std::filesystem::path> GetFilesInFolder(const std::filesystem::path& strFolderPath, const std::string& strSuffix);
}
