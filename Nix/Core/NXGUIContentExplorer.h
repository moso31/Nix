#pragma once
#include "Header.h"
#include <filesystem>

struct NXGUIContentExplorerSelectionInfo 
{
	NXGUIContentExplorerSelectionInfo() : bSelectedMask(false) {}
	bool bSelectedMask;
	std::filesystem::path filePath;
};

class NXGUIContentExplorer
{
public:
	NXGUIContentExplorer();
	~NXGUIContentExplorer() {}

	void Render();

	// �� ContentExplorer ��������νṹ�л��Ƶ����ļ���
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// �� ContentExplorer ��������νṹ�л���FolderPath�������������ļ���
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	std::filesystem::path m_contentFilePath;

	std::unordered_map<size_t, NXGUIContentExplorerSelectionInfo> m_selectionInfo;
};