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

	// �������ʲ�������ContentExplorer����ļ�ʱȷ�� �������š�
	// �ж�һ�µ�ǰFolder��������չ������Ϊ strSuffix ���ļ�������ļ����� strJudge + [��������] ����ʽ������������֡�
	// �������ʱ��ȷ�� �����Ǹ�����+1����û�������ļ�����ʹ�� 1��
	// Ȼ���ڵ�ǰ�ļ����·������·����
	std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge);
}
