#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include "NXInstance.h"

// 全局文件拖放管理器
// 用于处理从 Windows 资源管理器拖入的文件
class NXWinFileDrop : public NXInstance<NXWinFileDrop>
{
public:
	NXWinFileDrop() = default;
	~NXWinFileDrop() = default;

	// 添加拖入的文件路径（由 WM_DROPFILES 调用）
	void AddDroppedFile(const std::filesystem::path& filePath)
	{
		m_droppedFiles.push_back(filePath);
	}

	// 添加多个拖入的文件路径
	void AddDroppedFiles(const std::vector<std::filesystem::path>& filePaths)
	{
		m_droppedFiles.insert(m_droppedFiles.end(), filePaths.begin(), filePaths.end());
	}

	// 获取拖入的文件列表（不清空）
	const std::vector<std::filesystem::path>& GetDroppedFiles() const
	{
		return m_droppedFiles;
	}

	// 清空拖入的文件列表
	void ClearDroppedFiles()
	{
		m_droppedFiles.clear();
	}

	// 检查是否有待处理的拖入文件
	bool HasDroppedFiles() const
	{
		return !m_droppedFiles.empty();
	}

	// 获取拖入文件数量
	size_t GetDroppedFileCount() const
	{
		return m_droppedFiles.size();
	}

private:
	std::vector<std::filesystem::path> m_droppedFiles;
};

#define NXFileDrop NXWinFileDrop::GetInstance()