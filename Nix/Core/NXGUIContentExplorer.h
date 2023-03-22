#pragma once
#include "Header.h"
#include <filesystem>

struct NXGUIContentExplorerButtonDrugData
{
	std::filesystem::path srcPath;
};

struct NXGUIContentExplorerListSelectionInfo 
{
	NXGUIContentExplorerListSelectionInfo() : bSelectedMask(false) {}
	bool bSelectedMask;
	std::filesystem::path filePath;
};

class NXGUITexture;
class NXGUIContentExplorer
{
public:
	NXGUIContentExplorer(NXScene* pScene, NXGUITexture* pTexture);
	~NXGUIContentExplorer() {}

	void Render();

	// �� ContentExplorer ��������νṹ�л��Ƶ����ļ���
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// �� ContentExplorer ��������νṹ�л���FolderPath�������������ļ���
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	// ��ָ���ļ������ɲ�����Դ�ļ�
	void GenerateMaterialResourceFile(const std::filesystem::path& FolderPath); 

private:
	NXScene* m_pCurrentScene;
	std::filesystem::path m_contentFilePath;

	std::unordered_map<size_t, NXGUIContentExplorerListSelectionInfo> m_selectionInfo;
	NXGUIContentExplorerButtonDrugData m_btnDrugData;

	NXGUITexture* m_pGUITexture;
};