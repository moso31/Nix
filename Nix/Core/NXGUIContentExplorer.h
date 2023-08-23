#pragma once
#include "Header.h"
#include "NXGUICommon.h"

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

	// 在 ContentExplorer 的左侧树形结构中绘制单个文件夹
	void RenderContentFolder(const std::filesystem::path& FolderPath);

	// 在 ContentExplorer 的左侧树形结构中绘制FolderPath下属的所有子文件夹
	void RenderContentFolderList(const std::filesystem::path& FolderPath);

private:
	// 在指定文件夹生成材质资源文件
	void GenerateMaterialResourceFile(const std::filesystem::path& FolderPath); 
	void CreateMaterialFileOnDisk(const std::filesystem::path& matPath);

	void OnBtnContentLeftClicked(const std::filesystem::directory_entry& path);

private:
	NXScene* m_pCurrentScene;
	std::filesystem::path m_contentFilePath;

	std::unordered_map<size_t, NXGUIContentExplorerListSelectionInfo> m_selectionInfo;
	NXGUIAssetDragData m_btnDrugData;

	NXGUITexture* m_pGUITexture;
};