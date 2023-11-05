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

	// 在 ContentExplorer 的左侧树形结构中绘制单个文件夹
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// 在 ContentExplorer 的左侧树形结构中绘制FolderPath下属的所有子文件夹
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	// 在指定文件夹生成各种资产文件
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