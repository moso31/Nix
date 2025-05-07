#pragma once
#include <filesystem>
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

struct NXGUIAssetDragData
{
	std::filesystem::path srcPath;
};

class NXGUIFileBrowser;
namespace NXGUICommon
{
	void RenderSmallTextureIcon(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

	// ���ַ���ת����GUIStyle
	NXMSE_CBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);

	// ��ȡGUIStyle��Ӧ����������
	UINT				GetValueNumOfGUIStyle(NXMSE_CBufferStyle eGuiStyle);

	// ��ȡGUIStyle��Ӧ��Ĭ��ֵ
	Vector2				GetGUIParamsDefaultValue(NXMSE_CBufferStyle eGUIStyle);

	std::string ConvertShaderResourceDataToNSLParam(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay);

	// �����������ʲ�ʱ��ContentExplorer����ļ�ʱȷ�� ��������
	// ����ĳ���ļ������½��˲��ʣ�Ӧ�ý� "New Material ?"���������������ȷ�� ? �Ƕ��١�
	std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge);

	// ����ָ���ļ����µ������ļ���ע���ǵݹ��������������һ��������չ��Ϊ strSuffix ���ļ����顣
	// 2023.11.4 �˷�����Ŀǰ��Ҫ�ǲ��ʱ༭���У��滻 nssprof ��ʱ�򣬶�Ӧ���滻popupʹ�á����������������ʵȣ�Ҳ������������
	std::vector<std::filesystem::path> GetFilesInFolder(const std::filesystem::path& strFolderPath, const std::string& strSuffix);
}
