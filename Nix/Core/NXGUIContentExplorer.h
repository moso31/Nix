#pragma once
#include "NXGUICommon.h"
#include <filesystem>
#include <unordered_map>

struct NXGUIContentExplorerListSelectionInfo 
{
	NXGUIContentExplorerListSelectionInfo() : bSelectedMask(false) {}
	bool bSelectedMask;
	std::filesystem::path filePath;
};

class NXScene;
class NXGUIContentExplorer
{
public:
	NXGUIContentExplorer(NXScene* pScene);
	~NXGUIContentExplorer() {}

	void Render();

	// �� ContentExplorer ��������νṹ�л��Ƶ����ļ���
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// �� ContentExplorer ��������νṹ�л���FolderPath�������������ļ���
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	// ��ָ���ļ������ɸ����ʲ��ļ�
	void GenerateMaterialResourceFile(const std::filesystem::path& FolderPath); 
	void GenerateSSSProfileResourceFile(const std::filesystem::path& FolderPath);

	void CreateMaterialFileOnDisk(const std::filesystem::path& path);
	void CreateSSSProfileFileOnDisk(const std::filesystem::path& path);

	void OnBtnContentLeftClicked(const std::filesystem::directory_entry& path);

private:
	NXScene* m_pCurrentScene;
	std::filesystem::path m_contentFilePath;

	std::unordered_map<size_t, NXGUIContentExplorerListSelectionInfo> m_selectionInfo;
	NXGUIAssetDragData m_btnDrugData;

	std::string m_strRename;
};