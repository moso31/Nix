#pragma once
#include "Header.h"
#include <filesystem>

class NXGUIContentExplorer
{
public:
	NXGUIContentExplorer();
	~NXGUIContentExplorer() {}

	void Render();
	void RenderContentListFolder(const std::filesystem::path& FolderPath, const std::string& strForceName);

private:
	std::filesystem::path m_contentFilePath;
	ImGuiTreeNodeFlags m_contentListTreeNodeFlags;
};