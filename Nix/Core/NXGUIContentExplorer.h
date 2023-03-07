#pragma once
#include "Header.h"
#include <filesystem>

class NXGUIContentExplorer
{
public:
	NXGUIContentExplorer();
	~NXGUIContentExplorer() {}

	void Render();

	// 在 ContentExplorer 的左侧树形结构中绘制单个文件夹
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// 在 ContentExplorer 的左侧树形结构中绘制FolderPath下属的所有子文件夹
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	std::filesystem::path m_contentFilePath;

	std::unordered_map<size_t, bool> m_bSelectionMask;
};