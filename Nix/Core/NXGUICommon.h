#pragma once
#include <filesystem>
#include "NXShaderDefinitions.h"

struct NXGUIAssetDragData
{
	std::filesystem::path srcPath;
};

class NXMaterial;
class NXGUIFileBrowser;
namespace NXGUICommon
{
	void RenderSmallTextureIcon(ImTextureID ImTexID, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

	// ���ַ���ת����GUIStyle
	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);

	// ��CBufferת����GUIStyle
	NXGUICBufferStyle	GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);

	// ��ȡGUIStyle��Ӧ����������
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);

	// ��ȡGUIStyle��Ӧ��Ĭ��ֵ
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

	std::string ConvertShaderResourceDataToNSLParam(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay);

	// �����������ʲ�ʱ��ContentExplorer����ļ�ʱȷ�� ��������
	// ����ĳ���ļ������½��˲��ʣ�Ӧ�ý� "New Material ?"���������������ȷ�� ? �Ƕ��١�
	std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge);

	// ����ָ���ļ����µ������ļ���ע���ǵݹ��������������һ��������չ��Ϊ strSuffix ���ļ����顣
	// 2023.11.4 �˷�����Ŀǰ��Ҫ�ǲ��ʱ༭���У��滻 nssprof ��ʱ�򣬶�Ӧ���滻popupʹ�á����������������ʵȣ�Ҳ������������
	std::vector<std::filesystem::path> GetFilesInFolder(const std::filesystem::path& strFolderPath, const std::string& strSuffix);
}
